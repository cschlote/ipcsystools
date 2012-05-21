#!/bin/bash
# DESCRIPTION: Script starts the LTE Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-lte[$$]"

LTE_CONNECTION_PID_FILE=/var/run/lte_connection.pid

LTE_DEV=`getipcoption connection.lte.dev`

#-----------------------------------------------------------------------
# Check for functional wwan0 interface
#-----------------------------------------------------------------------
function IsInterfaceAlive ()
{
	local pids
	local rc
	pids="`ip link show $LTE_DEV`"
	echo $pids | grep -q ",UP"
	rc=$?
	syslogger "debug" "status - wwan if: rc=$rc"
	return $rc
}

function StartWWANInterface ()
{
	if ! IsInterfaceAlive; then
		RefreshModemDevices
		local device=$COMMAND_DEVICE
		syslogger "info" "Starting wwan0 on modem device $device"
		/usr/sbin/chat -v -f $IPC_SCRIPTS_DIR/dip-umts.chat <$device >$device
		ifconfig $LTE_DEV up
	fi
}
function StopWANInterface ()
{
    if IsInterfaceAlive; then
		syslogger "info" "Stopping wwan0"
		ifconfig wwan0 down
		umtscardtool -s 'at!greset'
		sleep 5
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

    syslogger "info" "$LTE_DEV startet, wait for interface"
    StartWWANInterface
    sleep $sleeptime

    while [ true ] ; do
	#TODO: ip-up.d run-parts zur Signalisierung nutzen?
	if ifconfig | grep -q $LTE_DEV; then
	    syslogger "info" "$LTE_DEV available"
	    break
	fi

	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	syslogger "debug" "Waiting $LTE_DEV coming up"
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
	syslogger "debug" "Waiting for LTE network registration ($count_timeout/$ni_state)"

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


#-----------------------------------------------------------------------
# Main
#-----------------------------------------------------------------------
rc_code=0
obtainlock $LTE_CONNECTION_PID_FILE

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

			if ! IsInterfaceAlive; then
				if StartAndWaitForWANInterface; then
					WriteModemStatusFile ${MODEM_STATES[connected]}
				else
					syslogger "debug" "wwan interface didn't startup."
					$IPC_SCRIPTS_DIR/set_fp_leds 3g off
					rc_code=1
				fi
			else
				WriteModemStatusFile ${MODEM_STATES[connected]}
				syslogger "debug" "wwan interface is already running."
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
	StopWANInterface
	InitializeModem
	CheckNIState
    ;;
check)
	if IsInterfaceAlive; then
	    if [ $# -gt 1 ] && [ -n "$2" -a -n "$3" ]; then
		wan_ct=${2:=127.0.0.1}
		wan_gw=${3:=default}
		syslogger "debug" "Pinging check target $wan_ct via $wan_gw"

		if ping_target $wan_ct $wan_gw $LTE_DEV; then
		    syslogger "debug" "Ping to $wan_ct on WAN interface $LTE_DEV successful"
		else
		    syslogger "error" "Ping to $wan_ct on WAN interface $LTE_DEV failed"
		    rc_code=1;
		fi
	    else
		syslogger "error" "Missing ping target argument"
		rc_code=1;
	    fi
	else
	    syslogger "error" "PPPD isn't running, no LTE connection"
	    rc_code=2;
	fi
	;;
status)
	if IsInterfaceAlive; then
	    echo "Interface $LTE_DEV is active"
	else
	    echo "Interface $LTE_DEV isn't configured"
	    DetectModemCard
	    rc_code=1
	fi
	;;
*)	
	echo "Usage: $0 start|stop|check <ip> <gw>|status"
	exit 1
    ;;
esac

releaselock
exit $rc_code


