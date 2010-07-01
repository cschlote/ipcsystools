#!/bin/sh
# DESCRIPTION: Script starts the UMTS Connection
#       USAGE: $@ start | stop | check <ip>

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

# PID - File fuer das Skript
ETH_CONNECTION_PID_FILE=/var/run/eth_connection.pid

#-------------------------------------------------------------------------------
function IsETHAlive () {
    if [ "`ifconfig eth0 | grep UP | wc -l`" != "0" ] ; then
	return 0;
    fi
    return 1;
}

function StartETH () {
	syslogger "info" "ETH-Conn - Starting IF"
	if ! IsETHAlive; then
	    if [ "`mount | grep nfs | wc -l`" != "0" ]; then
		syslogger "debug" "ETH-Conn - NFS mounted rootfs. Don't touch eth0!"
	    else
		ifdown eth0 || true
		ifup eth0
	    fi
	fi
}
function StopETH () {
	syslogger "info" "ETH-Conn - Stopping eth0"
	if IsETHAlive; then
	    if [ "`mount | grep nfs | wc -l`" != "0" ]; then
		syslogger "debug" "ETH-Conn - NFS mounted rootfs. Don't touch eth0!"
	    else
		ifdown eth0
	    fi
	fi
}

#-----------------------------------------------------------------------

rc_code=0
obtainlock $ETH_CONNECTION_PID_FILE

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi

case "$cmd" in
    start)
	syslogger "debug" "ETH-Conn - starting connection..."
	StartETH
	;;
    stop)
	syslogger "debug" "ETH-Conn - stopping connection..."
    	StopETH
	;;
    check)	if [ $# -gt 1 ] && [ -n "$2" ]; then
		    syslogger "debug" "ETH-Conn - Pinging check target $2"
		    IsETHAlive &&
			ping -I eth0 -c 1 -w 3 $2 1>/dev/null  ||
			( sleep 5 &&
			ping -I eth0 -c 1 -w 3 $2 1>/dev/null ) ||
			( sleep 5 &&
			ping -I eth0 -c 1 -w 3 $2 1>/dev/null )
		    if [ $? != 0 ]; then
			syslogger "error" "ETH-Conn - Ping to $2 on WAN interface eth0 failed"
			rc_code=1;
		    else
			syslogger "debug" "ETH-Conn - Ping to $2 on WAN interface eth0 successful"
		    fi
		else
		    syslogger "debug" "ETH-Conn - Missing ping target argument"
		fi
	;;
    *)	echo "Usage: $0 start|stop|check <ip>"
	rc_code=1;
	;;
esac

releaselock
exit $rc_code

