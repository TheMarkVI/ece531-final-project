# ECE531 Final Project
Artip Nakchinda

# This project implements the following:
- A programmable thermostat emulation system that reads temperature data from a file (created by thermocouple).
- thermostat is programmable (at least 3 different temperature points over a day).
- system can be controlled remotely via an HTTP interface.
- The thermostat can report temperatures and status via HTTP.
- The heater can be turned on/off based on the programmed schedule and reported temperature, with actions logged to a known file with timestamps.

# Project Structure
- `thermo-serv.py`: Flask-based server code for managing thermostat devices.
- `thermo.c`: C program that emulates a thermostat by reading temperature data from a file and controlling a heater based on a programmable schedule.


# Cloud server implementation:
- A Flask-based server that manages thermostat devices, allowing users to register thermostats, set programs, and receive status updates.
- The server provides an HTTP interface for managing thermostat programs and receiving status updates.

# Qemu host:
- A C program that emulates a thermostat by reading temperature data from a file and controlling a heater based on a programmable schedule.
- The program can be controlled remotely via an HTTP interface, allowing users to set temperature programs and report status.



## Acknowledgements
- Google Gemini was used to assist with guidance and suggestions for implementation.
    - as well as some coding for efficiency's sake
