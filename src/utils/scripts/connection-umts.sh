#!/bin/sh
# DESCRIPTION: Script starts the UMTS Connection
#       USAGE: umts-connection.sh start || stop

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

# PID - File fuer das Skript
UMTS_CONNECTION_PID_FILE=/var/run/umts_connection.pid

UMTS_DEV=`getmcboption connection.umts.dev`

#-----------------------------------------------------------------------
function IsPPPDAlive () {
    local pids
    local netclass
    local rc
    pids="`pidof pppd`" && netclass="`systool -c net | grep -q $UMTS_DEV`"; rc=$?
    syslogger "debug" "UMTS-Conn - pppd processes: $pids $netclass ($rc)"		
    return $rc
}

function StartPPPD () {
    RefreshModemDevices
    local device=$CONNECTION_DEVICE
    syslogger "info" "UMTS-Conn - Starting pppd on modem device $device"
    pppd $device 460800 connect "/usr/sbin/chat -v -f $MCB_SCRIPTS_DIR/ppp-umts.chat" &
}
function StopPPPD () {
    local pids=`pidof pppd`
    syslogger "info" "UMTS-Conn - Stopping pppd ($pids)"
    if [ -z "$pids" ]; then
	    syslogger "info" "UMTS-Conn - No pppd is running."
	    return 0
    fi

    kill -TERM $pids > /dev/null
    if [ "$?" != "0" ]; then
	rm -f /var/run/ppp*.pid > /dev/null
    fi
}

function WaitForPPP0Device () {
    # Loop Counters
    local count_timeout=0
    local count_timeout_max=12
    local sleeptime=5
    local reached_timeout=0

    while [ true ] ; do
	if systool -c net | grep -q $UMTS_DEV; then
	    syslogger "info" "UMTS-Conn - $UMTS_DEV available"
	    WriteConnectionAvailableFile
	    break
	fi

	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	syslogger "debug" "UMTS-Conn - Waiting $UMTS_DEV coming up"		
	sleep $sleeptime
	count_timeout=$[count_timeout+1]
    done

    if [ $reached_timeout -eq 1 ]; then
	return 0
    else
	return 1
    fi
}

#
# Wait until modem is booked into service provider network
#
function WaitForModemBookedIntoNetwork () {
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
	syslogger "debug" "UMTS-Conn - Waiting for UMTS network registration ($count_timeout/$ni_state)"

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

rc_code=0
obtainlock $UMTS_CONNECTION_PID_FILE

wan_ct=${2:=127.0.0.1}
wan_gw=${3:=default}

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
    start)
	syslogger "info" "UMTS-Conn - starting connection..."
	rc_code=1
	ReadModemStatusFile
	if 	[ $MODEM_STATUS == ${MODEM_STATES[detectedID]} ] ||
		[ $MODEM_STATUS == ${MODEM_STATES[readyID]} ] ||
		[ $MODEM_STATUS == ${MODEM_STATES[registeredID]} ]; then

	    # LED 3g Timer blinken
	    $MCB_SCRIPTS_DIR/leds.sh 3g timer

	    # Check for modem booked into network
	    if WaitForModemBookedIntoNetwork; then
		sleep 1
		WriteConnectionFieldStrengthFile
		WriteConnectionNetworkModeFile

		IsPPPDAlive
		if [ $? -eq 1 ]; then			
		    StartPPPD
		    WaitForPPP0Device
		    if [ $? -eq 1 ]; then		
			WriteModemStatusFile ${MODEM_STATES[connected]}
			rc_code=0;
		    else
			syslogger "debug" "UMTS-Conn - ppp deamon didn't startup."
			$MCB_SCRIPTS_DIR/leds.sh 3g off
		    fi
		else
		    syslogger "debug" "UMTS-Conn - ppp deamon is already running."
		    rc_code=0;
		fi
	    else
		syslogger "error" "UMTS-Conn - Could not initialize datacard (timeout)"
		$MCB_SCRIPTS_DIR/leds.sh 3g off
		$UMTS_FS
		syslogger "info" "UMTS-Conn - reported fieldstrength is $?."			  
	    fi
	else
	    syslogger "debug" "UMTS-Conn - modem not ready"
	fi
    ;;
    stop)
	syslogger "info" "UMTS-Conn - stopping connection..."
	StopPPPD
	CheckNIState
    ;;
    check)
	if IsPPPDAlive; then
	    if [ $# -gt 1 ] && [ -n "$wan_ct" -a -n "$wan_gw" ]; then
		syslogger "debug" "ETH-Conn - Pinging check target $wan_ct via $wan_gw"
		ip route add $wan_ct/32 via $wan_gw dev $UMTS_DEV;
		ip route
		ping -I $UMTS_DEV -c 1 -W 10 -w 60 $wan_ct 1>/dev/null ||
		( sleep 10 &&
		ping -I $UMTS_DEV -c 3 -W 15 -w 60 $wan_ct 1>/dev/null) ||
		( sleep 10 &&
		ping -I $UMTS_DEV -c 5 -W 20 -w 60 $wan_ct 1>/dev/null)
		if [ $? != 0 ]; then
		    syslogger "error" "UMTS-Conn - Ping to $wan_ct on WAN interface $UMTS_DEV failed"
		    rc_code=1;
		else
		    syslogger "debug" "UMTS-Conn - Ping to $wan_ct on WAN interface $UMTS_DEV successful"
		fi
		ip route del $wan_ct/32 via $wan_gw dev $UMTS_DEV;
	    else
		syslogger "error" "UMTS-Conn - Missing ping target argument"
	    fi
	else
		syslogger "error" "UMTS-Conn - PPPD isn't running, no UMTS connection"
	fi
	;;
    status)
	if IsPPPDAlive; then
	    echo "Interface $UMTS_DEV is active"
	else
	    echo "Interface $UMTS_DEV isn't configured"; rc_code=1
	fi
	;;
    *)	echo "Usage: $0 start|stop|check <ip> <gw>|status"
	exit 1
    ;;
esac

releaselock
exit $rc_code


