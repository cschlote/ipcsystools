#!/bin/sh
# DESCRIPTION: Script starts the UMTS Connection
#       USAGE: $@ start | stop | check <ip>

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

# PID - File fuer das Skript
ETH_CONNECTION_PID_FILE=/var/run/eth_connection.pid

#-------------------------------------------------------------------------------
function IsETHAlive () {
    ifconfig eth0 | grep UP | wc -l
    if [ $? = 0 ]; then
	return 1
    else
	return 0
    fi
}

function StartETH () {
	syslogger "info" "ETH-Conn - Starting IF"
	
}
function StopETH () {
	syslogger "info" "UMTS-Conn - Stopping pppd ($pids)"
}

#-----------------------------------------------------------------------

rc_code=0
obtainlock $UMTS_CONNECTION_PID_FILE

case "$1" in
    start)	StartETH	;;
    stop)	StopETH		;;
    check)	if [ -n "$2" ]; then
		    ping -I eth0 -c 1 $2
		    if [ ! $? ]; then
			syslogger "error" $DESC "Ping to $wan_ct on WAN interface $wan_if failed"
			rc_code=1;
		    fi
	;;
    *)	echo "Usage: $0 start|stop|check <ip>"
	rc_code=1;
	;;
esac

releaselock
exit $rc_code

