#!/bin/bash
# DESCRIPTION: Script starts an ETH Connection
#       USAGE: $0 start | stop | check <ip> <gw> | status

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="connection-eth[$$]"

ETH_CONNECTION_PID_FILE=/var/run/eth_connection.pid

ETH_DEV=`getipcoption connection.eth.dev`
ETH_KEEPUP=`getipcoption connection.eth.keepup`

#-----------------------------------------------------------------------
function IsETHAlive ()
{
    if ! ip addr show dev $ETH_DEV | grep -q "inet " ; then
	syslogger "error" "status - interface $ETH_DEV has no ipv4 addr"
	return 1;
    fi
    
    #TODO: Link - Überwachung einführen?
    
    if ! ifconfig $ETH_DEV | grep -q UP ; then
	syslogger "warn" "status - interface $ETH_DEV is not up"
	return 1;
    fi
    syslogger "debug" "status - interface $ETH_DEV is up and running"
    return 0;
}

function IsNotNFSRoot ()
{
    if [ "$ETH_DEV" = "eth0" ] && mount | grep -q ^/dev/root.*nfs.*; then
	syslogger "debug" "NFS mounted rootfs. Don't touch $ETH_DEV"
	return 1
    fi
    return 0
}

function StartETH ()
{
    syslogger "info" "Starting $ETH_DEV"
    if ! IsETHAlive; then
	if IsNotNFSRoot; then
	    ifdown $ETH_DEV -fv
	    ifup $ETH_DEV -v
	fi
    fi
}
function StopETH ()
{
	syslogger "info" "Stopping $ETH_DEV"
	if [ $ETH_KEEPUP != "1" ] && IsNotNFSRoot; then
		syslogger "info" "Stopping $ETH_DEV"
    		ifdown $ETH_DEV -fv
	fi
}

#-----------------------------------------------------------------------

rc_code=0
obtainlock $ETH_CONNECTION_PID_FILE

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
    start)
	syslogger "debug" "starting connection..."
	StartETH
	# Restart IPSec
    	restart_vpn_connection    
	;;
    stop)
	syslogger "debug" "stopping connection..."
    	StopETH
	;;
    check)
	if IsETHAlive; then
	    if [ $# -gt 2 ] && [ -n "$2" -a -n "$3" ]; then
		wan_ct=${2:=127.0.0.1}
		wan_gw=${3:=default}
		syslogger "debug" "Pinging check target $wan_ct via $wan_gw"

		if ping_target $wan_ct $wan_gw $ETH_DEV; then
		    syslogger "debug" "Ping to $2 on WAN interface $ETH_DEV successful"
		else
		    syslogger "error" "Ping to $2 on WAN interface $ETH_DEV failed"
		    rc_code=1;
		fi
	    else
		syslogger "debug" "missing argss for 'check' interface $ETH_DEV"
		rc_code=1
	    fi
	else
	    syslogger "error" "interface $ETH_DEV not ready"
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
    *)	echo "Usage: $0 start|stop|check <ip> <gw>|status"
	rc_code=1;
	;;
esac

releaselock
exit $rc_code

