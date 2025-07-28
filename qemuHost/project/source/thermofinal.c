// Artip Nakchinda
// ECE 531 - Assn 3: Your Own Daemon!
// Note: Heavy use of Google Gemini for code guidance & assistance 
//          + commands from lecture material
// 
// Compilation:
//  For x86_64: make -f makefile-x86_64 
//  For ARM:    make -f makefile-arm
//  *Check that your buildroot directory is correctly mapped in the Makefiles.
//
// Manual Compilation:
//  For x86_64: gcc -o thermoclient  thermofinal.c 
//  For ARM: $(BUILDROOT_DIRECTORY)/output/host/usr/bin/arm-linux-gcc  -o thermoclient thermofinal.c
// 
// When daemon is running: 
// For x86:$ journalctl -f // see that daemon is outputting in log  
// For ARM (buildroot):$ tail -f /var/log/messages
// 
// To kill daemon process:
// $ killall hw-timeDaemon
// or 
// $ kill <PID of timeDaemon>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <curl/curl.h>

// --- STRUCTS ---
typedef struct {
    char server_url[256];
    char thermostat_id[64];
    char temp_file_path[256];
    char status_file_path[256];
} AppConfig;
struct MemoryStruct {
    char *memory;
    size_t size;
};
typedef struct { // hold single point in program
    int hour;
    int minute;
    float temp;
} ProgramPoint;


// --- FUNCTION PROTOTYPES ---
static void signal_handler(const int signal);
void print_help();
int parse_config(const char* filename, AppConfig* config);
float read_current_temperature(const char* filename);
void update_heater_status(const char* filename, int heaterOn);
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
int get_program_from_server(const AppConfig* config, struct MemoryStruct* chunk);
int post_status_to_server(const AppConfig* config, float currTemp, int heaterOn);
float get_target_temp_from_program(const char* program_json);

// --- MAIN CODE ---
int main(int argc, char *argv[]) {
    char *configPath = "/etc/thermostat.conf";
    const char* tempFile = "/tmp/temp";
    const char* statusFile = "/tmp/status";

    // arg parsing
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                configPath = strdup(optarg);
                break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
            case '?':
            default:
                exit(EXIT_FAILURE);
        }
    }


    // Daemonization Process
    pid_t pid = fork();
    if(pid < 0) {
      exit(EXIT_FAILURE);
    } 
    if(pid > 0) {
      exit(EXIT_SUCCESS);
    }
    if(setsid() < 0) {
      exit(EXIT_FAILURE);
    }

    // Hardening Process
    if(chdir("/") < 0) {
      exit(EXIT_FAILURE);
    }
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, SIG_IGN);
    umask(0);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Thermo main daemon process
    // Set up syslog
    openlog("thermoclient", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Thermostat daemon starting with config file: %s", configPath);

    AppConfig config = {0}; // Initialize config
    int heaterOn = 0; // Heater status 0 = OFF, 1 = ON
    float hysteresis = 0.5; // prevent rapid switching of heater state

    while(1) {
        // read config file
        if(parse_config(configPath, &config) != 0) {
            syslog(LOG_ERR, "Failed to parse config file: %s", configPath);
            sleep(10); // Wait before retrying
            continue;
        }

        // Log the server URL and thermostat ID
        syslog(LOG_INFO, "Config: Server URL: %s, ID=%s", config.server_url, config.thermostat_id);
        // Read current temperature
        float currentTemp = read_current_temperature(tempFile);
        syslog(LOG_INFO, "Current temperature: %.2f", currentTemp);

        // GET program from cloud server
        struct MemoryStruct chunk = { .memory = malloc(1), .size = 0};
        if (get_program_from_server(&config, &chunk) != 0) {
            syslog(LOG_ERR, "Failed to get program from server.");
            free(chunk.memory);
            sleep(10);
            continue;
        }

        // Determine target temp
        float targetTemp = get_target_temp_from_program(chunk.memory);
        free(chunk.memory);
        syslog(LOG_INFO, "Current target temperature: %.2f", targetTemp);

        // compare current temp with target temp; decide heater state
        if(heaterOn) {
            // if heater is ON, shut off if temp is above target
            if(currentTemp > targetTemp + hysteresis) {
                heaterOn = 0;
            }
        }
        else {
            // heater OFF, turn ON if temp is below target
            if(currentTemp < targetTemp - hysteresis) {
                heaterOn = 1;
            }
        }

        // write new heater state to /tmp/status file
        update_heater_status(statusFile, heaterOn);

        // POST current status to server
        post_status_to_server(&config, currentTemp, heaterOn);

        sleep(5); 
    }

    syslog(LOG_INFO, "Thermo daemon shutting down.");
    closelog();
    if (strcmp(configPath, "/etc/thermostat.conf") != 0) {
        free(configPath);
    }
    return EXIT_SUCCESS;
}
// --- MAIN CODE ---

// Functions
static void signal_handler(const int signal) {
    switch (signal) {
        case SIGHUP:
            syslog(LOG_INFO, "Received SIGHUP signal. Configuration will be reloaded on next loop.");
            break;
        case SIGTERM:
            syslog(LOG_INFO, "Received SIGTERM, shutting down gracefully.");
            closelog();
            exit(EXIT_SUCCESS);
            break;
    }
}

void print_help() {
    printf("Usage: thermo [options]\n");
    printf("Options:\n");
    printf("  -h, --help          Show help message\n");
    printf("  -c, --config FILE   Specify configuration file\n");
}

int parse_config(const char* filename, AppConfig* config) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        syslog(LOG_ERR, "Cannot open config file: %s", filename);
        return -1;
    }
    char line[512];
    int found_url = 0, found_id = 0, found_temp_path = 0, found_status_path = 0;
    while (fgets(line, sizeof(line), file)) {
        char key[256], value[256];
        if(sscanf(line, "%255[^=]=%255s", key, value) == 2) {
            if(strcmp(key, "SERVER_URL") == 0) {
                strncpy(config->server_url, value, sizeof(config->server_url) - 1);
                found_url = 1;
            } 
            else if(strcmp(key, "THERMOSTAT_ID") == 0) {
                strncpy(config->thermostat_id, value, sizeof(config->thermostat_id) - 1);
                found_id = 1;
            } 
            else if(strcmp(key, "TEMP_FILE") == 0) { 
                strncpy(config->temp_file_path, value, sizeof(config->temp_file_path) - 1);
                found_temp_path = 1;
            } 
            else if(strcmp(key, "STATUS_FILE") == 0) {
                strncpy(config->status_file_path, value, sizeof(config->status_file_path) - 1);
                found_status_path = 1;
            }
        }
    }
    fclose(file);
    if (found_url && found_id && found_temp_path && found_status_path) {
        return 0;
    } 
    else {
        syslog(LOG_ERR, "Config file is missing one or more required keys.");
        return -1;
    }

    fclose(file);

    if(found_url && found_id) {
        return 0; // Success
    } else {
        syslog(LOG_ERR, "Config file missing required keys (SERVER_URL, THERMOSTAT_ID).");
        return -1; // Failure
    }
}

float read_current_temperature(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        syslog(LOG_ERR, "Cannot open temperature file: %s", filename);
        return -999.0; 
    }
    float temp = -999.0;
    fscanf(file, "%f", &temp);
    fclose(file);
    return temp;
}

void update_heater_status(const char* filename, int heaterOn) {
    FILE* file = fopen(filename, "w"); 
    if (!file) {
        syslog(LOG_ERR, "Cannot open status file for writing: %s", filename);
        return;
    }
    const char* action = heaterOn ? "ON" : "OFF";
    long timestamp = (long)time(NULL);
    fprintf(file, "%s : %ld\n", action, timestamp);
    fclose(file);
    syslog(LOG_INFO, "Heater status updated to: %s", action);
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) { return 0; }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

int get_program_from_server(const AppConfig* config, struct MemoryStruct* chunk) { 
    CURL *curl;
    CURLcode res;
    char url[512];
    snprintf(url, sizeof(url), "%s/thermostat/%s/program", config->server_url, config->thermostat_id);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            syslog(LOG_ERR, "GET request failed: %s", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_easy_cleanup(curl);
        return 0;
    }
    return -1;
}

int post_status_to_server(const AppConfig* config, float currTemp, int heaterOn) { 
    CURL *curl;
    CURLcode res;
    char url[512];
    char post_data[256];
    snprintf(url, sizeof(url), "%s/thermostat/%s/status", config->server_url, config->thermostat_id);
    snprintf(post_data, sizeof(post_data), "{\"current_temp\": %.2f, \"heater_on\": %s}", currTemp, heaterOn ? "true" : "false");

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            syslog(LOG_ERR, "POST request failed: %s", curl_easy_strerror(res));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return 0;
    }
    return -1;
}

float get_target_temp_from_program(const char* program_json) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    int current_hour = local_time->tm_hour;
    int current_minute = local_time->tm_min;

    ProgramPoint program[3];
    int count = 0;
    
    // Manual parsing of the JSON array
    const char* ptr = program_json;
    while ((ptr = strchr(ptr, '{')) && count < 3) {
        int hour, minute;
        float temp;
        // This is more robust: it doesn't assume the order of keys.
        // It looks for both "time" and "temp" within the same object.
        const char* time_ptr = strstr(ptr, "\"time\"");
        const char* temp_ptr = strstr(ptr, "\"temp\"");
        const char* end_obj_ptr = strchr(ptr, '}');

        if (time_ptr && temp_ptr && end_obj_ptr && time_ptr < end_obj_ptr && temp_ptr < end_obj_ptr) {
            if (sscanf(time_ptr, "\"time\": \"%d:%d\"", &hour, &minute) == 2 &&
                sscanf(temp_ptr, "\"temp\": %f", &temp) == 1) {
                
                program[count].hour = hour;
                program[count].minute = minute;
                program[count].temp = temp;
                count++;
            }
        }
        ptr = end_obj_ptr; // Move pointer to the end of the current object
    }

    if (count == 0) {
        syslog(LOG_ERR, "Could not parse any program points from server response.");
        return 20.0; // Return a safe default
    }

    // Find the latest setpoint that is in the past
    ProgramPoint active_setpoint = program[0];
    int latest_time_val = -1;

    for (int i = 0; i < count; i++) {
        int setpoint_time_val = program[i].hour * 60 + program[i].minute;
        int current_time_val = current_hour * 60 + current_minute;

        if (setpoint_time_val <= current_time_val) {
            if (setpoint_time_val > latest_time_val) {
                latest_time_val = setpoint_time_val;
                active_setpoint = program[i];
            }
        }
    }
    
    // If no setpoint was in the past (e.g., it's 02:00 and first setpoint is 07:00),
    // we should use the latest setpoint from the previous day.
    if (latest_time_val == -1) {
        ProgramPoint latest_of_all = program[0];
        int latest_overall_time = program[0].hour * 60 + program[0].minute;
        for (int i = 1; i < count; i++) {
             int setpoint_time_val = program[i].hour * 60 + program[i].minute;
             if (setpoint_time_val > latest_overall_time) {
                latest_overall_time = setpoint_time_val;
                latest_of_all = program[i];
            }
        }
        active_setpoint = latest_of_all;
    }

    return active_setpoint.temp;
}
