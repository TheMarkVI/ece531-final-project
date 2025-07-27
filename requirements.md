Goal: Emulating a thermostat
- program provided to write temps in a file (deg Celsius)
- pretend file is an actual thermocouple
- thermostat is programmable (no less than 3 different points over a day)
- program remotely via http interface
- report temperatures and status via http interface
- turn heater on/off based on program and reported temp (write to known file with timestamp)

How it works:
- P1 (thermocouple) -> file (temperature) -> P2 (Our program) -> HTTP
    - all in qemu host

Requirements for the project:
Your system shall:
- Read the current temperature from a known file.
    - /var/log/temperature
    - read a single temperature value written to file
    - float in degrees C
- Turn heat on/off based on program and current temperature.
    - /var/log/heater
    - turn heat on/off by writing to /var/log/heater
    - A single line <action> : <timestamp>
    - action:= <on|off> timestamp:= <posix time of action>

- Start a daemon service that can also run from command line
- process a config file
    - default option, also supplied to program via -c & config_file flags (e.g. -c <config_file> or --config_file <config_file>)
- Provide a help option (-h or --help)
    - This will print typical help for the application

- The configuration file shall configure:
    - Service endpoint (e.g. https://<some_host>:8000)
    - log files (e.g. /my/logfile/here, for program output)
    - any other config files
- accept programs via an http interface
    - program up to three different temps for a day set at arbitrary times

- Report status to an outside process via http
- report actions to an outside process via http

Your system may: (optional)
- support more extensive programming
