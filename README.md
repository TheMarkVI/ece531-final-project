# ECE531 Final Project
Artip Nakchinda

## Usage
### Thermostat Server
The web service is running on an AWS EC2 instance. Access the service with the following URLs: (this service will run for a limited time).
- `http://3.22.227.202:8080/`
- `http://ec2-3-22-227-202.us-east-2.compute.amazonaws.com:8080/`

### Thermostat Client
To run the thermostat client, you need to compile the C program and run it on a QEMU host. The client reads temperature data from a file and controls the heater based on a programmable schedule.

### Compiling the client
For x86_64: 
```bash
gcc -o thermoclient thermofinal.c -lcurl -lpthread
```
For ARM:
```bash
$(BUILDROOT_DIRECTORY)/output/host/usr/bin/arm-linux-gcc -o thermoclient thermofinal.c -lcurl -lpthread
```


## This project implements the following:
- A programmable thermostat emulation system that reads temperature data from a file (created by thermocouple).
- thermostat is programmable (at least 3 different temperature points over a day).
- system can be controlled remotely via an HTTP interface.
- The thermostat can report temperatures and status via HTTP.
- The heater can be turned on/off based on the programmed schedule and reported temperature, with actions logged to a known file with timestamps.

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
- Google Gemini was used to assist with guidance and suggestions for implementation.
    - as well as some coding for efficiency's sake
