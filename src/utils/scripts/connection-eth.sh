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
    if ifconfig $ETH_DEV | grep -q UP ; then
	syslogger "debug" "ETH-Conn - found configured $ETH_DEV"
	return 0;
    fi
    return 1;
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
	    ifdown $ETH_DEV || true
	    ifup $ETH_DEV
	fi
    fi
}
function StopETH () {
    syslogger "info" "ETH-Conn - Stopping $ETH_DEV"
    if IsETHAlive ; then
	if [ $ETH_KEEPUP != "1" ] && IsNotNFSRoot; then
	    ifdown $ETH_DEV
	fi
    fi
}

#-----------------------------------------------------------------------

rc_code=0
obtainlock $ETH_CONNECTION_PID_FILE

wan_ct=${2:=127.0.0.1}
wan_gw=${3:=default}

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
    check)	if [ $# -gt 2 ] && [ -n "$wan_ct" -a -n "$wan_gw" ]; then
		    syslogger "debug" "ETH-Conn - Pinging check target $wan_ct via $wan_gw"
		    ip route add $wan_ct/32 via $wan_gw dev $ETH_DEV;
		    IsETHAlive &&
			ping -I $ETH_DEV -c 1 -w 3 $wan_ct 1>/dev/null  ||
			( sleep 5 &&
			ping -I $ETH_DEV -c 1 -w 3 $wan_ct 1>/dev/null ) ||
			( sleep 5 &&
			ping -I $ETH_DEV -c 1 -w 3 $wan_ct 1>/dev/null )
		    if [ $? != 0 ]; then
			syslogger "error" "ETH-Conn - Ping to $2 on WAN interface $ETH_DEV failed"
			rc_code=1;
		    else
			syslogger "debug" "ETH-Conn - Ping to $2 on WAN interface $ETH_DEV successful"
		    fi
		    ip route del $wan_ct/32 via $wan_gw dev $ETH_DEV;
		else
		    syslogger "debug" "ETH-Conn - Missing ping target argument"
		fi
	;;
    status)
	if IsETHAlive; then
	    echo "Interface $ETH_DEV is active"
	else
	    echo "Interface $ETH_DEV isn't configured"; rc_code=1
	fi
	;;
    *)	echo "Usage: $0 start|stop|check <ip>|status"
	rc_code=1;
	;;
esac

releaselock
exit $rc_code

