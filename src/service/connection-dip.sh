#!/bin/bash
# DESCRIPTION: Script starts the DIP Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-dip[$$]"

DIP_CONNECTION_PID_FILE=$IPC_STATUSFILE_DIR/dip_connection.pid

DIP_DEV=`getipcoption connection.dip.dev`

DIP_LED=`getipcoption connection.dip.statusled`
DIP_LED=${DIP_LED:=3g}

#-----------------------------------------------------------------------
# Check for functional wwan0 interface
#-----------------------------------------------------------------------
# Query 'ip addr $DIP_DEV', check for UP and IPV4 address attributes
function IsInterfaceAlive ()
{
    local ifstatus
    local rc
    ifstatus="`ip addr show $DIP_DEV`"
    echo $ifstatus | grep -q " UP "; rc=$?
    syslogger "debug" "status - wwan if: $DIP_DEV  UP? (rc=$rc)"
    if [ $rc = 0 ]; then
	echo $ifstatus | grep -q "inet "; rc=$?
	syslogger "debug" "status - wwan if: $DIP_DEV  INET? (rc=$rc)"
    fi
    return $rc
}

#-----------------------------------------------------------------------
# Start and Stop DIP_DEV WAN interface
#-----------------------------------------------------------------------
# - Requires entry in /etc/network/interfaces for configuration!!!!
function StartWANInterface ()
{
    if ! IsInterfaceAlive; then
	ConfigureDIPMode
	RefreshModemDevices
	local device=$COMMAND_DEVICE
	syslogger "info" "Starting $DIP_DEV (AT Commands on $device)"
	ifdown $DIP_DEV || true
	sleep 3
	ifup $DIP_DEV 
	sleep 1
    fi
}
function StopWANInterface ()
{
    if IsInterfaceAlive; then
	syslogger "info" "Stopping $DIP_DEV"
	ifdown $DIP_DEV || true
	sleep 6
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

    syslogger "info" "$DIP_DEV started, wait for interface"
    StopWANInterface
    StartWANInterface
    sleep $sleeptime

    while [ true ] ; do
	if IsInterfaceAlive; then
	    syslogger "info" "$DIP_DEV available"
	    break
	fi

	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	syslogger "debug" "Waiting $DIP_DEV coming up"
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
function CreateDIPChatScript
{
    (
	cat <<EOF
REPORT CONNECT
ABORT BUSY
ABORT ERROR
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
SAY "Setup autoconnect for profile 1\n"
OK      AT!SCPROF=1,"DEFAULT",1,0,0,0
SAY "Activating profile 1 for APN connection\n"
OK      AT!SCACT=1,1
OK      ''
EOF
    ) >$IPC_STATUSFILE_DIR/dip-mode.chat
}

#
# Reconfigure modem for DIP connections and global software reset
#
function ConfigureDIPMode ()
{
    ConfigureDIPModeModemCfg

    echo "Reseting modem for interface $DIP_DEV."
    RefreshModemDevices
    umtscardtool -s 'at!greset'
    sleep 15
    echo "Modem is now configured for Autostart DirectIP. Use ipup/ifdown"
    echo "$DIP_DEV to startup/shutdown interface."
}

#
# Reconfigure modem for DIP connections - modem soft reset
#
function ConfigureDIPModeModemCfg ()
{
    echo "Configuring modem for interface $DIP_DEV for DirectIP."	    
    RefreshModemDevices
    CreateDIPChatScript
    /usr/sbin/chat -v -f $IPC_STATUSFILE_DIR/dip-mode.chat <$COMMAND_DEVICE >$COMMAND_DEVICE
    sleep 2
}
    
#-----------------------------------------------------------------------
# Main
#-----------------------------------------------------------------------
rc_code=0
obtainlock $DIP_CONNECTION_PID_FILE

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
start)
	syslogger "info" "starting connection..."
	ReadModemStatusFile
	if 	[ "$MODEM_STATUS" = "${MODEM_STATES[detectedID]}" ] ||
		[ "$MODEM_STATUS" = "${MODEM_STATES[readyID]}" ] ||
		[ "$MODEM_STATUS" = "${MODEM_STATES[registeredID]}" ]; then

	    # LED 3g Timer blinken
	    $IPC_SCRIPTS_DIR/set_fp_leds $DIP_LED timer

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
					$IPC_SCRIPTS_DIR/set_fp_leds $DIP_LED off
					rc_code=1
				fi
			else
				WriteModemStatusFile ${MODEM_STATES[connected]}
				$IPC_SCRIPTS_DIR/set_fp_leds $DIP_LED on
				syslogger "debug" "wwan interface is already running."
			fi
	    else
			syslogger "error" "Could not initialize datacard (timeout)"
			$IPC_SCRIPTS_DIR/set_fp_leds $DIP_LED off
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

		if ping_target $wan_ct $wan_gw $DIP_DEV; then
		    syslogger "debug" "Ping to $wan_ct on WAN interface $DIP_DEV successful"
		else
		    syslogger "error" "Ping to $wan_ct on WAN interface $DIP_DEV failed"
		    rc_code=1;
		fi
	    else
		syslogger "error" "Missing ping target argument"
		rc_code=1;
	    fi
	else
	    syslogger "error" "DIP isn't running, no mobile connection"
	    rc_code=2;
	fi
	;;
status)
	if IsInterfaceAlive; then
	    syslogger "debug"  "Interface $DIP_DEV is active"
	    echo "Interface $DIP_DEV is active"
	else
	    syslogger "error"  "Interface $DIP_DEV isn't configured"
	    echo "Interface $DIP_DEV isn't configured"
	    DetectModemCard
	    rc_code=1
	fi
	;;
config)
	if IsInterfaceAlive; then
	    StopWANInterface
	fi
	ConfigureDIPMode 
	;;
*)	
	echo "Usage: $0 start|stop|check <ip> <gw>|status|config"
	exit 1
    ;;
esac

releaselock
exit $rc_code

