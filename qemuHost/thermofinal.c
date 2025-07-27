// Artip Nakchinda
// ECE 531 - Assn 3: Your Own Daemon!
// Note: Hevay use of Google Gemini for code guidance/assistance 
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

// structs
typedef struct {
    char server_url[256];
    char thermostat_id[64];
} AppConfig;

// Function prototypes
static void signal_handler(const int signal);
void print_help();
int parse_config(const char* filename, AppConfig* config);

// --- MAIN CODE ---
int main(int argc, char *argv[]) {
    char *config_path = "/etc/thermostat.conf"; // Default config path
    
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
                config_path = strdup(optarg);
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
    syslog(LOG_INFO, "Thermostat daemon starting with config file: %s", config_path);

    AppConfig config = {0}; // Initialize config
    while(1) {
        // Parse config file
        if(parse_config(config_path, &config) != 0) {
            syslog(LOG_ERR, "Failed to parse config file: %s", config_path);
            sleep(5); // Wait before retrying
            continue;
        }

        // Log the server URL and thermostat ID
        syslog(LOG_INFO, "Config: Server URL: %s, ID=%s", config.server_url, config.thermostat_id);

        sleep(5); 
    }

    syslog(LOG_INFO, "Thermo daemon shutting down.");
    closelog();
    if (strcmp(config_path, "/etc/thermostat.conf") != 0) {
        free(config_path);
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
    exit(0);
}

int parse_config(const char* filename, AppConfig* config) {
        FILE* file = fopen(filename, "r");
    if (!file) {
        syslog(LOG_ERR, "Cannot open config file: %s", filename);
        return -1;
    }

    char line[512];
    int found_url = 0;
    int found_id = 0;

    while (fgets(line, sizeof(line), file)) {
        char key[256], value[256];
        // sscanf will parse the line into two strings separated by '='
        if (sscanf(line, "%255[^=]=%255s", key, value) == 2) {
            if (strcmp(key, "SERVER_URL") == 0) {
                strncpy(config->server_url, value, sizeof(config->server_url) - 1);
                found_url = 1;
            } else if (strcmp(key, "THERMOSTAT_ID") == 0) {
                strncpy(config->thermostat_id, value, sizeof(config->thermostat_id) - 1);
                found_id = 1;
            }
        }
    }

    fclose(file);

    if (found_url && found_id) {
        return 0; // Success
    } else {
        syslog(LOG_ERR, "Config file missing required keys (SERVER_URL, THERMOSTAT_ID).");
        return -1; // Failure
    }
}