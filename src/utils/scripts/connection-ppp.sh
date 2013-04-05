#!/bin/bash
# DESCRIPTION: Script starts the PPP Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-ppp[$$]"

PPP_CONNECTION_PID_FILE=$IPC_STATUSFILE_DIR/ppp_connection.pid

PPP_DEV=`getipcoption connection.ppp.dev`

#-----------------------------------------------------------------------
# Check for functional pppd and pppx interface
#-----------------------------------------------------------------------
function IsPPPDAlive ()
{
    local pids
    local rc

    pids="`pidof pppd`"; rc=$?

    syslogger "debug" "status - pppd: $pids (rc=$rc)"
    return $rc
}

function StartPPPD ()
{
    if ! IsPPPDAlive; then
	RefreshModemDevices
	local device=$CONNECTION_DEVICE
	syslogger "info" "Starting pppd on modem device $device"
	pppd $device 460800 connect "/usr/sbin/chat -v -f $IPC_SCRIPTS_DIR/ppp-mode.chat" &
    fi
}
function StopPPPD ()
{
    if IsPPPDAlive; then
		local pids=`pidof pppd`
		syslogger "info" "Stopping pppd ($pids)"
		if [ -z "$pids" ]; then
			syslogger "info" "No pppd is running."
			return 0
		fi

		kill -TERM $pids > /dev/null
		if [ "$?" != "0" ]; then
			rm -f /var/run/ppp*.pid > /dev/null
		fi
    fi
}

#
# Start the PPPD connection and retry if it fails
#
function StartAndWaitForPPPD ()
{
    # Loop Counters
    local count_timeout=0
    local count_timeout_max=12
    local sleeptime=5
    local reached_timeout=0

    syslogger "info" "$PPP_DEV startet, wait for interface"
    StartPPPD
    sleep $sleeptime

    while [ true ] ; do
	#TODO: ppp.ip-up.d run-parts zur Signalisierung nutzen?
	if systool -c net | grep -q $PPP_DEV; then
	    syslogger "info" "$PPP_DEV available"
	    break
	fi

	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	syslogger "debug" "Waiting $PPP_DEV coming up"
	sleep $sleeptime
	count_timeout=$[count_timeout+1]
    done

    return $reached_timeout
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

    SetSIMPIN
    #TODO: Set operator selection

    # Check for modem booked into network
    CheckNIState
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
	CheckNIState
	ni_state=$?
	count_timeout=$[count_timeout+1]
    done
    return $reached_timeout
}

#
# Wait until modem is booked into service provider network
#
function ConfigurePPPMode ()
{
    echo "Configuring modem for interface $PPP_DEV for DirectIP."	    
    RefreshModemDevices
    /usr/sbin/chat -v -f $IPC_SCRIPTS_DIR/ppp-mode.chat <$COMMAND_DEVICE >$COMMAND_DEVICE
    sleep 2
    echo "Reseting modem for interface $PPP_DEV."	    
    umtscardtool -s 'at!greset'
    sleep 2
    echo "Modem is now configured for ppp-deamon- Use pon/poff for "
    echo "$PPP_DEV to startup interface."
}
    

#-----------------------------------------------------------------------
# Main
#-----------------------------------------------------------------------
rc_code=0
obtainlock $PPP_CONNECTION_PID_FILE

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
start)
	syslogger "info" "starting connection..."
	ReadModemStatusFile
	if 	[ "$MODEM_STATUS" = "${MODEM_STATES[detectedID]}" ] ||
		[ "$MODEM_STATUS" = "${MODEM_STATES[readyID]}" ] ||
		[ "$MODEM_STATUS" = "${MODEM_STATES[registeredID]}" ]; then

	    # LED 3g Timer blinken
	    $IPC_SCRIPTS_DIR/set_fp_leds 3g timer

	    # Check for modem booked into network
	    if WaitForModemBookedIntoNetwork; then
			sleep 1
			WriteConnectionFieldStrengthFile
			WriteConnectionNetworkModeFile

			if ! IsPPPDAlive; then
				if StartAndWaitForPPPD; then
				WriteModemStatusFile ${MODEM_STATES[connected]}
				else
				syslogger "debug" "ppp deamon didn't startup."
				$IPC_SCRIPTS_DIR/set_fp_leds 3g off
				rc_code=1
				fi
			else
				WriteModemStatusFile ${MODEM_STATES[connected]}
				syslogger "debug" "ppp deamon is already running."
			fi
	    else
			syslogger "error" "Could not initialize datacard (timeout)"
			$IPC_SCRIPTS_DIR/set_fp_leds 3g off
			$UMTS_FS
			syslogger "info" "reported fieldstrength is $?."
			rc_code=1
	    fi
	else
	    syslogger "debug" "modem in status $MODEM_STATUS, won't start again"
	fi
    ;;
stop)
	syslogger "info" "stopping connection..."
	StopPPPD
	InitializeModem
	CheckNIState
    ;;
check)
	if IsPPPDAlive; then
	    if [ $# -gt 1 ] && [ -n "$2" -a -n "$3" ]; then
		wan_ct=${2:=127.0.0.1}
		wan_gw=${3:=default}
		syslogger "debug" "Pinging check target $wan_ct via $wan_gw"

		if ping_target $wan_ct $wan_gw $PPP_DEV; then
		    syslogger "debug" "Ping to $wan_ct on WAN interface $PPP_DEV successful"
		else
		    syslogger "error" "Ping to $wan_ct on WAN interface $PPP_DEV failed"
		    rc_code=1;
		fi
	    else
		syslogger "error" "Missing ping target argument"
		rc_code=1;
	    fi
	else
	    syslogger "error" "PPPD isn't running, no mobile connection"
	    rc_code=2;
	fi
	;;
status)
	if IsPPPDAlive; then
	    echo "Interface $PPP_DEV is active"
	else
	    echo "Interface $PPP_DEV isn't configured"
	    DetectModemCard
	    rc_code=1
	fi
	;;
config)
	if IsInterfaceAlive; then
	    StopPPPD
	fi
	ConfigurePPPMode 
	;;
    *)	echo "Usage: $0 start|stop|check <ip> <gw>|status"
	exit 1
    ;;
esac

releaselock
exit $rc_code


