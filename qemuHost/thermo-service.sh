#!/bin/sh
# S99thermoserv: start thermocouple sim and client daemon at boot

# guidance:
# <stuff that always happens>
# <start function>
# <stop function>
# <case handling args>
# exit $?

# The full path to your thermostat client executable
THERMO_CLIENT="/usr/bin/thermoclient"
# thermocouple simulation executable
THERMO_SIM="/usr/bin/tcsimd"

# Names for client and simulation processes
CLIENT_NAME="thermoclient"
SIM_NAME="tcsimd"

# Function to start the services
start() {
    printf "Starting Thermocouple Simulation: "
    # Start the simulation in the background
    start-stop-daemon --start --background --exec $THERMO_SIM
    echo "OK"

    printf "Starting Thermocouple Client Daemon: "
    # Start your client daemon in the background
    start-stop-daemon --start --background --exec $THERMO_CLIENT
    echo "OK"
}

# Function to stop the services
stop() {
    printf "Stopping Thermocouple Client Daemon: "
    start-stop-daemon --stop --quiet --name $CLIENT_NAME
    echo "OK"

    printf "Stopping Thermocouple Simulation: "
    start-stop-daemon --stop --quiet --name $SIM_NAME
    echo "OK"
}

restart() {
    stop
    start
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart|reload)
        restart
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit 0
