#!/bin/sh
# DESCRIPTION: Script starts the UMTS Connection
#       USAGE: $@ start | stop | check <ip>

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

# PID - File fuer das Skript
ETH_CONNECTION_PID_FILE=/var/run/eth_connection.pid

ETH_DEV=`getmcboption connection.eth.dev`
ETH_KEEPUP=`getmcboption connection.eth.keepup`

#-----------------------------------------------------------------------
function IsETHAlive () {
    if ! ip addr show dev $ETH_DEV | grep -q "inet " ; then
	syslogger "error" "ETH-Conn - status - interface $ETH_DEV has no ipv4 addr"
	return 1;
    fi
    if ! mii-diag -s $ETH_DEV 2>&1 >/dev/null; then
	syslogger "error" "ETH-Conn - status - interface $ETH_DEV has no link beat"
	return 1;
    fi
    if ! ifconfig $ETH_DEV | grep -q UP ; then
	syslogger "warn" "ETH-Conn - status - interface $ETH_DEV is not up"
	return 1;
    fi
    syslogger "debug" "ETH-Conn - status - interface $ETH_DEV is up and running"
    return 0;
}
function IsNotNFSRoot () {
    if [ "$ETH_DEV" = "eth0" ] && mount | grep -q ^/dev/root.*nfs.*; then
	syslogger "debug" "ETH-Conn - NFS mounted rootfs. Don't touch $ETH_DEV"
	return 1
    fi
    return 0
}

function StartETH () {
    syslogger "info" "ETH-Conn - Starting $ETH_DEV"
    if ! IsETHAlive; then
	if IsNotNFSRoot; then
	    ifdown $ETH_DEV -fv
	    ifup $ETH_DEV -v
	fi
    fi
}
function StopETH () {
    syslogger "info" "ETH-Conn - Stopping $ETH_DEV"
#    if IsETHAlive ; then
	if [ $ETH_KEEPUP != "1" ] && IsNotNFSRoot; then
    syslogger "info" "ETH-Conn - yyyStopping $ETH_DEV"
	    ifdown $ETH_DEV -fv
	fi
#    fi
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
    check)
	if IsETHAlive; then
	    if [ $# -gt 2 ] && [ -n "$2" -a -n "$3" ]; then
		wan_ct=${2:=127.0.0.1}
		wan_gw=${3:=default}
		syslogger "debug" "ETH-Conn - Pinging check target $wan_ct via $wan_gw"
		ping_target $wan_ct $wan_gw $ETH_DEV;
		if [ $? != 0 ]; then
		    syslogger "error" "ETH-Conn - Ping to $2 on WAN interface $ETH_DEV failed"
		    rc_code=1;
		else
		    syslogger "debug" "ETH-Conn - Ping to $2 on WAN interface $ETH_DEV successful"
		fi
	    else
		syslogger "debug" "ETH-Conn - missing argss for 'check' interface $ETH_DEV"
		rc_code=1
	    fi
	else
	    syslogger "error" "ETH-Conn - interface $ETH_DEV not ready"
	    rc_code=1
	fi
	;;
    status)
	if IsETHAlive; then
	    echo "Interface $ETH_DEV is active"
	else
	    echo "Interface $ETH_DEV isn't configured"
	    rc_code=1
	fi
	;;
    *)	echo "Usage: $0 start|stop|check <ip>|status"
	rc_code=1;
	;;
esac

releaselock
exit $rc_code

