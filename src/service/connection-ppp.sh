#!/bin/bash
# DESCRIPTION: Script starts the PPP Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-ppp[$$]"

PPP_CONNECTION_PID_FILE=$IPC_STATUSFILE_DIR/ppp_connection.pid

PPP_DEVUNIT=`getipcoption connection.ppp.dev.unit`
PPP_DEVUNIT=${PPP_DEVUNIT:=10}

#PPP_DEV=`getipcoption connection.ppp.dev`
#PPP_DEV=${PPP_DEV:=ppp$PPP_DEVUNIT}
PPP_DEV=ppp$PPP_DEVUNIT
PPP_LED=`getipcoption connection.ppp.statusled`
PPP_LED=${PPP_LED:=3g}

#-----------------------------------------------------------------------
# Check for functional pppd and pppx interface
#-----------------------------------------------------------------------
function PPPProcessExists ()
{
    ps ax | grep -v grep | grep -q "pppd /dev/.*ppp-mode.chat"
    return $?
}

function IsPPPDAlive ()
{
    # Check for running PPPD with given config for umts
    if ! PPPProcessExists ; then
	syslogger "warn" "status - PPPD for umts modem is not up"
	return 1;
    fi

    # Test for existing ppp$PPP_DEVUNIT interface - FIXME hardwired code!!!!
    # TODO: Find out the real name of ppp interface for connection
    if ! ifconfig | grep -q $PPP_DEV ; then
	syslogger "warn" "status - no $PPP_DEV interface found"
	return 2;
    fi
    if ! ip addr show dev $PPP_DEV | grep -q "inet " ; then
	syslogger "error" "status - interface $PPP_DEV has no ipv4 addr"
	return 2;
    fi
    syslogger "debug" "status - ppp/umts connection ready"
    return 0;
}

function StartPPPD ()
{
    local auth=`getipcoption sim.auth`
    local user=`getipcoption sim.username`
    local password=`getipcoption sim.passwd`
    local pppopts=
    
    if ! IsPPPDAlive; then
	RefreshModemDevices
	local device=$CONNECTION_DEVICE
	syslogger "info" "Starting pppd on modem device $device"
	CreatePPPChatScript
	if [ "$auth" -eq 1 -a -n "$user" -a -n "$password" ] ; then
	    syslogger "info" "Passing APN user and password to PPPD"
	    pppopts="user $user password $password"
	fi
	pppd $device 460800 unit $PPP_DEVUNIT connect "/usr/sbin/chat -v -f $IPC_STATUSFILE_DIR/ppp-mode.chat" $pppopts &
    fi
}
function StopPPPD ()
{
    if IsPPPDAlive; then
	local pid
	local file=/var/run/$PPP_DEV.pid
	if [ -e $file ]; then
	    pid=`cat $file`
	fi
	if [ -z "$pid" ]; then
	    syslogger "info" "No pppd is running."
	    return 0
	fi

	syslogger "info" "Stopping pppd/umts ($pid)"
	kill -TERM $pid > /dev/null
	if [ "$?" != "0" ]; then
	    rm -f /var/run/$PPP_DEV.pid > /dev/null
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
# Create a chatscript (only used for configuration)
#
function CreatePPPChatScript
{
    (
	cat <<EOF
REPORT CONNECT
ABORT BUSY
ABORT ERROR
ABORT "NO CARRIER"
TIMEOUT 120
SAY "Resetting modem\n"
''      ATZ
SAY "Setting APN name for profile 1\n"
EOF
	echo "OK      AT+CGDCONT=1,\"IP\",\"`getipcoption sim.apn`\""
	if [ `getipcoption sim.auth` -eq 1 ]; then
	    echo "SAY \"Setup password and username for APN\n\""
	    echo "OK      AT\$QCPDPP=1,1,\"`getipcoption sim.passwd`\",\"`getipcoption sim.username`\""
	else
	    echo "OK      AT\$QCPDPP=1,0"
	fi
	cat <<EOF
SAY "Dialout to provider APN/ppp service, 120 second timeout\n"
OK      ATDT*99***1#
CONNECT ''
EOF
    ) >$IPC_STATUSFILE_DIR/ppp-mode.chat
}


#
# Reconfigure modem for PPP mode
#
function ConfigurePPPMode ()
{
    echo "Configuring modem for interface $PPP_DEV for DirectIP."	    
    RefreshModemDevices
    CreatePPPChatScript
    /usr/sbin/chat -v -f $IPC_STATUSFILE_DIR/ppp-mode.chat <$COMMAND_DEVICE >$COMMAND_DEVICE
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
	    $IPC_SCRIPTS_DIR/set_fp_leds $PPP_LED timer

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
				$IPC_SCRIPTS_DIR/set_fp_leds $PPP_LED off
				rc_code=1
				fi
			else
				WriteModemStatusFile ${MODEM_STATES[connected]}
				$IPC_SCRIPTS_DIR/set_fp_leds $PPP_LED on
				syslogger "debug" "ppp deamon is already running."
			fi
	    else
			syslogger "error" "Could not initialize datacard (timeout)"
			$IPC_SCRIPTS_DIR/set_fp_leds $PPP_LED off
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
	if IsPPPDAlive; then
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


