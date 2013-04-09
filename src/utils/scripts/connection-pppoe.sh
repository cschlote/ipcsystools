#!/bin/bash
# DESCRIPTION: Script starts the PPPoE Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-pppoe[$$]"

PPPOE_CONNECTION_PID_FILE=$IPC_STATUSFILE_DIR/pppoe_connection.pid

PPPOE_DEV=`getipcoption connection.pppoe.dev`
PPPOE_CFG=`getipcoption connection.pppoe.cfg`
PPPOE_IF=`getipcoption connection.pppoe.if`
PPPOE_DEV=${PPPOE_DEV:=ppp0}
PPPOE_IF=${PPPOE_IF:=eth1}
PPPOE_KEEPUP=`getipcoption connection.pppoe.keepup`

#-----------------------------------------------------------------------
# Check for functional pppd and pppx interface
#-----------------------------------------------------------------------
function IsPPPoEAlive ()
{
    # Check for physical PPPoE ethernet interface 
    if ! ifconfig $PPPOE_IF | grep -q UP ; then
	syslogger "warn" "status - interface $PPPOE_IF is not up"
	return 1;
    fi
    syslogger "debug" "status - physical interface $PPPOE_IF is up and running"

    # Check for running PPPD with given config as started by pon/poff
    if ! ps ax | grep -q "pppd.*$PPPOE_CFG" ; then
	syslogger "warn" "status - PPPD for config $PPPOE_CFG is not up"
	return 1;
    fi

    # Test for existing ppp0 interdace - FIXME hardwired code!!!!
    # TODO: Find out the real name of ppp interface for connection
    if ! ifconfig | grep -q $PPPOE_DEV ; then
	syslogger "warn" "status - no $PPPOE_DEV interface found"
	return 2;
    fi
    if ! ip addr show dev $PPPOE_DEV | grep -q "inet " ; then
	syslogger "error" "status - interface $PPPOE_DEV has no ipv4 addr"
	return 2;
    fi
    syslogger "debug" "status - pppoe connection ready"
    return 0;
}

function StartPPPD ()
{
    if ! IsPPPoEAlive; then
	syslogger "info" "Starting pppoe profile $PPPOE_CFG"
	ifup br0
	# Check for running PPPD with given config as started by pon/poff
	if ps ax | grep -q "pppd.*$PPPOE_CFG" ; then
	    syslogger "warn" "status - PPPD for config $PPPOE_CFG is up - terminate"
	    poff $PPPOE_CFG
	fi
	pon $PPPOE_CFG
    fi
}
function StopPPPD ()
{
    if IsPPPoEAlive; then
	if [ $PPPOE_KEEPUP -eq 1 ] ; then
	    syslogger "info" "Stopping pppoe profile $PPPOE_CFG"
	    poff $PPPOE_CFG
	else 
	    syslogger "info" "Keeping pppoe profile $PPPOE_CFG active"
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
    local count_timeout_max=6
    local sleeptime=1
    local reached_timeout=0

    syslogger "info" "Starting PPPoE $PPPOE_DEV on $PPPOE_IF, waiting for interface"
    StartPPPD
    sleep $sleeptime

    while [ true ] ; do
	#TODO: ppp.ip-up.d run-parts zur Signalisierung nutzen?
	if IsPPPoEAlive; then
	    syslogger "info" "PPPoE seems to be available"
	    break
	fi

	if [ $count_timeout -ge $count_timeout_max ]; then
	    reached_timeout=1
	    break
	fi

	syslogger "debug" "Waiting PPPoE coming up"
	sleep $sleeptime
	count_timeout=$[count_timeout+1]
    done

    return $reached_timeout
}


#
# Wait until modem is booked into service provider network
#
function ConfigurePPPoE ()
{
    echo "Configuring PPPoE profile $PPPOE_CFG for interface $PPPOE_IF."
    pppoeconf
    echo "PPPoE subsystem is now configured- Use pon/poff $PPPOE_CFG"
    echo "to control interface."
}


#-----------------------------------------------------------------------
# Main
#-----------------------------------------------------------------------
rc_code=0
obtainlock $PPPOE_CONNECTION_PID_FILE

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
    start)
	syslogger "info" "starting connection..."
	# LED 3g Timer blinken
	$IPC_SCRIPTS_DIR/set_fp_leds 3g timer

	if ! IsPPPoEAlive; then
	    if StartAndWaitForPPPD; then
   		syslogger "debug" "pppoe connection did startup."
	    else
		syslogger "debug" "pppoe connection didn't startup."
		$IPC_SCRIPTS_DIR/set_fp_leds 3g off
		rc_code=1
	    fi
	else
	    $IPC_SCRIPTS_DIR/set_fp_leds 3g on
	    syslogger "debug" "pppoe connection is already running."
	fi
	;;
    stop)
	syslogger "info" "stopping connection..."
	StopPPPD
	$IPC_SCRIPTS_DIR/set_fp_leds 3g off
	;;
    check)
	if IsPPPoEAlive; then
	    if [ $# -gt 1 ] && [ -n "$2" -a -n "$3" ]; then
		wan_ct=${2:=127.0.0.1}
		wan_gw=${3:=default}
		syslogger "debug" "Pinging check target $wan_ct via $wan_gw on interface $PPPOE_DEV"
		if ping_target $wan_ct $wan_gw $PPPOE_DEV; then
		    syslogger "debug" "Ping to $wan_ct on PPPoE interface $PPPOE_DEV successful"
		else
		    syslogger "error" "Ping to $wan_ct on PPPoE interface $PPPOE_DEV failed"
		    rc_code=1;
		fi
	    else
		syslogger "error" "Missing ping target argument"
		rc_code=1;
	    fi
	else
	    syslogger "error" "PPPoE isn't running, no connection"
	    rc_code=2;
	fi
	;;
    status)
	if IsPPPoEAlive; then
	    echo "PPPoE Interface $PPPOE_DEV is active"
	else
	    echo "PPPoE Interface $PPPOE_DEV isn't configured"
	    rc_code=1
	fi
	;;
    config)
	if IsPPPoEAlive; then
	    StopPPPD
	fi
	ConfigurePPPoE
	;;
    *)	echo "Usage: $0 start|stop|check <ip> <gw>|status"
	exit 1
    ;;
esac

releaselock
exit $rc_code


