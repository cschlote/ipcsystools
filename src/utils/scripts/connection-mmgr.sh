#!/bin/bash
#-----------------------------------------------------------------------
# DESCRIPTION: Script starts the ModemManager Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-mmgr[$$]"

MMGR_CONNECTION_PID_FILE=$IPC_STATUSFILE_DIR/mmgr_connection.pid

MMGR_DEV=`getipcoption connection.mmgr.dev`
MMGR_DEV=${MMGR_DEV:=wwan0}
MMGR_LED=`getipcoption connection.mmgr.statusled`
MMGR_LED=${MMGR_LED:=3g}

MMGR_PATH=
MMGR_SIM_PATH=

#-----------------------------------------------------------------------
# Get the first modem path from modemmanger. Return error RC when
# no modem ist found
#-----------------------------------------------------------------------
function GetModemPath ()
{
    local tmp=`mmcli -L | grep -E "/org/.*" | cut -d" " -f1 | cut -f2`
    if [ -n 'tmp' ]; then
	MMGR_PATH=$tmp
	syslogger "debug" "Found modem on path path $MMGR_PATH."
    else
	syslogger "error" "No modem found by modemmanager."
	return 1
    fi
    return 0
}

#-----------------------------------------------------------------------
# Get the first modem path from modemmanger. Return error RC when
# no modem ist found
#-----------------------------------------------------------------------
function GetModemSIMPath ()
{
    local tmp=`mmcli -m $MMGR_PATH | grep -o -E "SIM.*|.*path: (.*)" | grep -o "/org/.*" | sed "s/'//g"`
    if [ -n 'tmp' ]; then
	MMGR_SIM_PATH=$tmp
	syslogger "debug" "Found modem SIM on path path $MMGR_SIM_PATH."
    else
	syslogger "error" "No modem found by modemmanager."
	return 1
    fi
    return 0
}

#-----------------------------------------------------------------------
# Check for functional wwan0 interface
#-----------------------------------------------------------------------
# Query 'ip addr $MMGR_DEV', check for UP and IPV4 address attributes
function IsInterfaceAlive ()
{
    local ifstatus
    local rc=1
    if GetModemPath; then
	# Check for connected modem
	ifstatus="`mmcli -m $MMGR_PATH --simple-status`"
	echo $ifstatus | grep -q "connected"; rc=$?
	syslogger "debug" "(rc=$rc) Is connected state on $MMGR_DEV"
	if [ $rc = 0 ]; then
	    # Check for assigned ip
	    ifstatus="`ip addr show $MMGR_DEV`"
	    echo $ifstatus | grep -q "inet "; rc=$?
	    syslogger "debug" "(rc=$rc) Is a ip defined for $MMGR_DEV"
	fi
    fi
    return $rc
}

#-----------------------------------------------------------------------
# Start and Stop MMGR_DEV WAN interface
#-----------------------------------------------------------------------
# - Requires entry in /etc/network/interfaces for configuration!!!!
function StartWANInterface ()
{
    if GetModemPath && ! IsInterfaceAlive; then
	echo "Starting $MMGR_DEV on $MMGR_PATH"
	syslogger "info" "Starting $MMGR_DEV on $MMGR_PATH"
	syslogger "debug" " Shutdown $MMGR_DEV and dhcp client"
	ifdown $MMGR_DEV || true
	syslogger "debug" " Startup modem on path $MMGR_PATH"
	StartModemManagerConnection
	syslogger "debug" " Startup $MMGR_DEV and dhcp client"
	ifup $MMGR_DEV
	syslogger "debug" " Setup default route to interface $MMGR_DEV"
	route del default
	route add default dev wwan0
    fi
}
function StopWANInterface ()
{
    if GetModemPath && IsInterfaceAlive; then
	echo "Stopping $MMGR_DEV on $MMGR_PATH"
	syslogger "info" "Stopping $MMGR_DEV"
	syslogger "debug" " Shutdown $MMGR_DEV and dhcp client"
	ifdown $MMGR_DEV || true
	syslogger "debug" " Stop modem on path $MMGR_PATH"
	StopModemManagerConnection
	syslogger "debug" " Clear default route to interface $MMGR_DEV"
	route del default
    fi
}

#
# Start the wwan0 connection and retry if it fails
#
function StartAndWaitForWANInterface ()
{
    # Loop Counters
    local count_timeout=0
    local count_timeout_max=12
    local sleeptime=5
    local reached_timeout=0

    syslogger "info" "$MMGR_DEV started, wait for interface"
    StopWANInterface
    StartWANInterface
    sleep $sleeptime

    while [ true ] ; do
	if IsInterfaceAlive; then
	    syslogger "info" "$MMGR_DEV available"
	    break
	fi

	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	syslogger "debug" "Waiting $MMGR_DEV coming up"
	sleep $sleeptime
	count_timeout=$[count_timeout+1]
    done

    return $reached_timeout
}

function MMgrSetSIMPIN ()
{
	local sim_pin=`getipcoption sim.pin`

	# Check for locked SIM
	ifstatus="`mmcli -m $MMGR_PATH --simple-status`"
	echo $ifstatus | grep -q "locked"; rc=$?
	syslogger "debug" " (rc=$rc) Is locked state on $MMGR_DEV"
	if [ $rc = 0 ] && [ -n "$sim_pin" ]; then
	    GetModemSIMPath
	    mmcli -i $MMGR_SIM_PATH
	    mmcli -i $MMGR_SIM_PATH --pin="$sim_pin"
	fi
	mmcli -i $MMGR_SIM_PATH
	local pin_state=$?
# FIXME better RC codes
	#  1: PIN angegeben, musste aber nicht gesetzt werden
	#  2: SIM Karte wurde nicht erkannt
	#  3: Der PIN wird benötigt, wurde aber nicht angegeben
	#  4: PUK oder SuperPIN benötigt. SIM-Karte entnehmen und mit einem Mobiltelefon entsperren.
	#  5: Die eingegebene PIN war falsch.
	#  6: Der AT-Befehl zum Setzen der PIN hat einen Fehler erzeugt.
	case $pin_state in
		0|1)
			WriteModemStatusFile ${MODEM_STATES[readyID]} ;;
		2)
			WriteModemStatusFile ${MODEM_STATES[sim_not_insertedID]} ;;
		3)
			WriteModemStatusFile ${MODEM_STATES[sim_pinID]} ;;
		*)
			WriteModemStatusFile "errorcode: $pin_state" ;;
	esac
}
function MMgrCheckNIState ()
{
	# Check for disabled modem and enable
	ifstatus="`mmcli -m $MMGR_PATH --simple-status`"
	echo $ifstatus | grep -q "disabled"; rc=$?
	syslogger "debug" " (rc=$rc) Is disabled state on $MMGR_DEV"
	if [ $rc = 0 ]; then
	    mmcli -m $MMGR_PATH -e
	    return $rc
	fi

	echo $ifstatus | grep -q "registered"; rc=$?
	syslogger "debug" " (rc=$rc) Is registered state on $MMGR_DEV"
	if [ $rc = 0 ]; then
	    WriteModemStatusFile ${MODEM_STATES[registeredID]}
#FIXME	    WriteGSMConnectionInfoFiles
	    return $rc
	fi

	echo $ifstatus | grep -q "connected"; rc=$?
	syslogger "debug" " (rc=$rc) Is connected state on $MMGR_DEV"
	if [ $rc = 0 ]; then
	    WriteModemStatusFile ${MODEM_STATES[registeredID]}
#FIXME	    WriteGSMConnectionInfoFiles
	    return $rc
	fi
	return $rc
}

#
# Wait until modem is booked into service provider network
#
function WaitForModemBookedIntoNetwork ()
{
    # Loop Counters
    local count_timeout=0
    local count_timeout_max=12
    local sleeptime=5
    local reached_timeout=0

    MMgrSetSIMPIN
    #TODO: Set operator selection

    # Check for modem booked into network
    MMgrCheckNIState
    local ni_state=$?
    while [ $ni_state -ne 0 ]; do
	syslogger "debug" "Waiting for mobile network registration ($count_timeout/$ni_state)"

	# Increase number of tries, when 'limited service' is reported
	# (ni_state==2)
	if [ $ni_state -eq 2 ]; then
	    count_timeout_max=18
	fi

	# Check for timeout reached
	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	# Sleep and retry
	sleep $sleeptime
	MMgrCheckNIState
	ni_state=$?
	count_timeout=$[count_timeout+1]
    done
    return $reached_timeout
}

#
# Control the modem
#
function StartModemManagerConnection
{
    echo "Setting APN name, and user/pw if needed. Startup connection."
    syslogger "debug" " Startup network connection on $MMGR_PATH"
    if [ `getipcoption sim.auth` -eq 1 ]; then
	mmcli -m $MMGR_PATH --simple-connect="apn=`getipcoption sim.apn`,user=`getipcoption sim.username`,password=`getipcoption sim.passwd`"
    else
	mmcli -m $MMGR_PATH --simple-connect="apn=`getipcoption sim.apn`"
    fi
}
function StopModemManagerConnection
{
    echo "Stop Connection"
    syslogger "debug" " Stoping network connection on $MMGR_PATH"
    mmcli -m $MMGR_PATH --simple-disconnect
}

#
# Reconfigure modem for MMGR connections and global software reset
#
function ConfigureMMGRMode ()
{
    echo "Reseting modem for interface $MMGR_DEV."
    syslogger "debug" " Stoping network connection on $MMGR_PATH"
    RefreshModemDevices
    mmcli -m $MMGR_PATH -d || true
    mmcli -m $MMGR_PATH -r || true
    echo "Modem is now configured for QMI. Use ModemManger and ipup/ifdown"
    echo "$MMGR_DEV to startup/shutdown interface."
}


#-----------------------------------------------------------------------
# Main
#-----------------------------------------------------------------------

rc_code=0
obtainlock $MMGR_CONNECTION_PID_FILE

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
start)
	syslogger "info" "starting connection..."
	ReadModemStatusFile
	if 	[ "$MODEM_STATUS" = "${MODEM_STATES[detectedID]}" ] ||
		[ "$MODEM_STATUS" = "${MODEM_STATES[readyID]}" ] ||
		[ "$MODEM_STATUS" = "${MODEM_STATES[registeredID]}" ]; then

	    # LED 3g Timer blinken
	    $IPC_SCRIPTS_DIR/set_fp_leds $MMGR_LED timer

	    # Check for modem booked into network
	    if GetModemPath && WaitForModemBookedIntoNetwork; then
		sleep 1
#FIXME		WriteConnectionFieldStrengthFile
#FIXME		WriteConnectionNetworkModeFile

		if ! IsInterfaceAlive; then
		    if StartAndWaitForWANInterface; then
			$IPC_SCRIPTS_DIR/set_fp_leds $MMGR_LED on
			WriteModemStatusFile ${MODEM_STATES[connected]}
		    else
			syslogger "debug" "wwan interface didn't startup."
			$IPC_SCRIPTS_DIR/set_fp_leds $MMGR_LED off
			rc_code=1
		    fi
		else
		    WriteModemStatusFile ${MODEM_STATES[connected]}
		    $IPC_SCRIPTS_DIR/set_fp_leds $MMGR_LED on
		    syslogger "debug" "wwan interface is already running."
		fi
	    else
		syslogger "error" "Could not initialize datacard (timeout)"
		$IPC_SCRIPTS_DIR/set_fp_leds $MMGR_LED off
#FIXME		$UMTS_FS
		syslogger "info" "reported fieldstrength is $?."
		rc_code=1
	    fi
	else
	    syslogger "debug" "modem in status $MODEM_STATUS, won't start again"
	fi
    ;;
stop)
	syslogger "info" "stopping connection..."
	StopWANInterface
	mmcli -m $MMGR_PATH -r || true
	MMgrCheckNIState
	$IPC_SCRIPTS_DIR/set_fp_leds $MMGR_LED off
    ;;
check)
	syslogger "info" "Checking ping target for connection..."
	if IsInterfaceAlive; then
	    if [ $# -gt 1 ] && [ -n "$2" -a -n "$3" ]; then
		wan_ct=${2:=127.0.0.1}
		wan_gw=${3:=default}
		syslogger "debug" "Pinging check target $wan_ct via $wan_gw"

		if ping_target $wan_ct $wan_gw $MMGR_DEV; then
		    syslogger "debug" "Ping to $wan_ct on WAN interface $MMGR_DEV successful"
		else
		    syslogger "error" "Ping to $wan_ct on WAN interface $MMGR_DEV failed"
		    rc_code=1;
		fi
	    else
		syslogger "error" "Missing ping target argument"
		rc_code=1;
	    fi
	else
	    syslogger "error" "MMGR isn't running, no mobile connection"
	    rc_code=2;
	fi
	;;
status)
	syslogger "info" "Checking status for connection..."
	if IsInterfaceAlive; then
	    syslogger "debug"  "Interface $MMGR_DEV is active"
	    echo "Interface $MMGR_DEV is active"
	else
	    syslogger "error"  "Interface $MMGR_DEV isn't configured"
	    echo "Interface $MMGR_DEV isn't configured"
	    DetectModemCard
	    rc_code=1
	fi
	;;
config)
	syslogger "info" "Configure modem hardware for connection..."
	if IsInterfaceAlive; then
	    StopWANInterface
	fi
	ConfigureMMGRMode
	;;
*)
	echo "Usage: $0 start|stop|check <ip> <gw>|status|config"
	exit 1
    ;;
esac

releaselock
exit $rc_code
