# ECE531 Final Project
Author: Artip Nakchinda

## Overview: This project implements the following:
- A programmable thermostat emulation system that reads temperature data from a file (from thermocouple).
- thermostat is programmable (at least 3 different temperature points over a day).
- system can be controlled remotely via an HTTP interface.
- The thermostat can report temperatures and status via HTTP.
- The heater can be turned on/off based on the programmed schedule and reported temperature, with actions logged to a known file with timestamps.

## Usage
### Thermostat Server
The web service is running on an AWS EC2 instance. Access the service with the following URLs: (this service will run for a limited time).
- `http://3.22.227.202:5031/`
- `http://ec2-3-22-227-202.us-east-2.compute.amazonaws.com:5031/`

### Thermostat Client (QEMU Host)
You can run the thermostat client in the `images` directory with the following shell file:
```bash
$ ./project.sh
```
This will start the QEMU emulator with the thermostat client and thermocouple already running. The client will connect to the cloud server and start sending temperature data.

Notes: 
- The QEMU host may take a while to start up.
- Make sure that the buildroot output directory is properly set in the ./project.sh file.


### Inside the Thermostat Client (QEMU Host)
- Log in with the provided credentials.
- Once logged in, there are four files in the home directory:
    - `S99thermo-service.sh`: This is the service that runs the thermostat client.
    - `thermoclient`: This is the main client program. (options in included)
    - `tcsimd`: This is a simulated thermocouple (no options)
    - `thermostat.conf`: This is the configuration file for the thermostat client.
        - It contains the server URL, thermostat ID, and log file paths.
- You can also take a look at the log files in the `/var/log/` directory to see the actions taken by the thermostat client.
    - `/var/log/messsages`: This file contains the log of actions taken by the thermostat client.
    - `/var/log/temp`: current temperature readings from the thermocouple.
    - `/var/log/status`: shows the status whether the heater is on or off.

#### S99thermo-service.sh
You can use the following file to start, stop, or restart the thermostat client service:
```bash
$ ./S99thermo-service.sh start      # Starts the service
$ ./S99thermo-service.sh stop       # Stops the service
$ ./S99thermo-service.sh restart    # Restarts the service
```

#### thermoclient
```bash
$ ./thermoclient -h
Usage: thermo [options]
Options:
  -h, --help          Show help message
  -c, --config FILE   Specify configuration file
```

#### Configuration File (thermostat.conf)
The configuration file should be formatted as follows:
```ini
SERVER_URL=<server-url>
THERMOSTAT_ID=<thermostat-id>
TEMP_FILE=<temp-file-path>
STATUS_FILE=<status-file-path>
```
There is a default configuration file loaded at startup, along with an existing thermostat ID. You can change the configuration file by using the `-c` option when running the `thermoclient`.


## HTTP Commands
You can use the following HTTP commands to interact with the thermostat server:
- `GET /thermostat/<thermostat_id>`: Get the current status of the thermostat.
- `POST /thermostat/`: Register a new thermostat with the server.
- `PUT /thermostat`: Update the thermostat program.
- `DELETE /thermostat`: Delete the thermostat from the server.

### Example Commands
#### Register a new thermostat:
```bash
$ curl -X POST -H "Content-Type: application/json" \
-d '{"location": "QEMU Final Project"}' \
http://3.22.227.202:5031/thermostat
```

#### Get the current status of the thermostat:
```bash
$ curl http://3.22.227.202:5031/thermostat/<thermostat-id>/program
```

#### Update the thermostat program (with a 3-point schedule):
```bash
$ curl -X PUT -H "Content-Type: application/json" \
-d '{"program": [{"time": "07:00", "temp": 21.0}, {"time": "14:00", "temp": 22.5}, {"time": "22:00", "temp": 19.0}]}' \
http://3.22.227.202:5031/thermostat/<thermostat-id>/program
```

#### Simulate status update from client:
```bash
$ curl -X POST -H "Content-Type: application/json" \
-d '{"current_temp": 21.8, "heater_on": true}' \
http://3.22.227.202:5031/thermostat/<thermostat-id>/last_status
```

#### Get full thermostat record:
```bash
$ curl http://3.22.227.202:5031/thermostat/<thermostat-id>
```

#### Delete a thermostat:
```bash
$ curl -X DELETE http://3.22.227.202:5031/thermostat/<thermostat-id>
```

## Manual compilation
If need be, you can manually compile the code in the source directory.
### Compiling the client
For x86_64: 
```bash
make -f makefile-x86_64 -o thermoclient thermofinal.c -lcurl -lpthread
```
For ARM:
```bash
make -f makefile-arm -o thermoclient thermofinal.c -lcurl -lpthread
```
### Running the server
You can run the server using Python. Make sure you have Flask and MongoDB installed and enabled.

## Project Structure
- `thermo-serv.py`: Flask-based server code for managing thermostat devices.
- `thermo.c`: C program that emulates a thermostat by reading temperature data from a file and controlling a heater based on a programmable schedule.

## Cloud server implementation:
- A Flask-based server that manages thermostat devices, allowing users to register thermostats, set programs, and receive status updates.
- The server provides an HTTP interface for managing thermostat programs and receiving status updates.

## qemu host:
- A C program that emulates a thermostat by reading temperature data from a file and controlling a heater based on a programmable schedule.
- The program can be controlled remotely via an HTTP interface, allowing users to set temperature programs and report status.

## Acknowledgements
- Google Gemini was heavily used to assist with guidance and suggestions for implementation.
    - as well as some coding for efficiency's sake
