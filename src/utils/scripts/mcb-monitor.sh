#!/bin/bash
#
# DESCRIPTION: Monitor the WAN and VPN Connections
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

DESC="mcb-monitor[$$]"

MCB_MONITOR_PID_FILE=/var/lock/mcb-monitor.pid

#-----------------------------------------------------------------------
# Read variables from config file
#-----------------------------------------------------------------------

# Sektion [WATCHDOG-CONNECTION]
CHECK_CONNECTION_REBOOT=`getmcboption watchdog.wan.connection.max_restarts`
MAX_CONNECTION_LOST=`getmcboption watchdog.wan.connection.max_time`

# Sektion [WATCHDOG-PING]
CHECK_PING_ENABLED=`getmcboption watchdog.ping.check_ping_enabled`
CHECK_PING_IP=`getmcboption watchdog.ping.check_ping_ip`
CHECK_PING_REBOOT=`getmcboption watchdog.ping.check_ping_reboot`
CHECK_PING_TIME=`getmcboption watchdog.ping.check_ping_time`

# Lokale Variablen fuer die Ueberwachung Internetverbindung (PPPD)
CONNECTION_FAULT_FILE=$MCB_STATUSFILE_DIR/connection_fault
if [ ! -e $CONNECTION_FAULT_FILE ]; then
  echo 0 > $CONNECTION_FAULT_FILE
fi
CONNECTION_FAULT=`cat $CONNECTION_FAULT_FILE`

# Lokale Variablen fuer die Ueberwachung externe Verbindung
PING_FAULT_FILE=$MCB_STATUSFILE_DIR/connection_ping_fault
if [ ! -e $PING_FAULT_FILE ]; then
  echo 0 > $PING_FAULT_FILE
fi
PING_FAULT=`cat $PING_FAULT_FILE`

# Zeitpunkt des letzten abgesetzten "ping"
LAST_PING_FILE=$MCB_STATUSFILE_DIR/last_ping_time

reset_wan=false
reset_vpn=false
reset_system=false

#-----------------------------------------------------------------------
# Ping some connection with timeinterval and max fail count
# - ping a target with a time interval
# - record number of fails
# - reset system of max number of fails
function check_connection_maxping ()
{
    local rc=0
    if [ $CHECK_PING_ENABLED -eq "1" ]; then
	local timestamp=`date +%s`
	local last_ping=`date +%s`
	if [ -e $LAST_PING_FILE ]; then
	    last_ping=`cat $LAST_PING_FILE`
	else
	    echo $last_ping > $LAST_PING_FILE
	fi

	# Check for next time to send a ping and count fails
	if [ $[$timestamp - $last_ping] -gt $CHECK_PING_TIME ]; then

	    echo `date +%s` > $LAST_PING_FILE

	    if ping -c 1 -W 5 -s 8 $CHECK_PING_IP > /dev/null ; then
		syslogger "debug" "Check ping peer $CHECK_PING_IP passed"
		PING_FAULT=0
	    else
		syslogger "info"  "Check ping peer $CHECK_PING_IP failed $PING_FAULT time(s)"
		PING_FAULT=$[$PING_FAULT+1]
	    fi
	    echo $PING_FAULT > $PING_FAULT_FILE

	    # Maximum number of pings sent?
	    if [ $PING_FAULT -ge $CHECK_PING_REBOOT ]; then
		syslogger "error" "FAIL: ping peer $CHECK_PING_IP unavailable $CHECK_PING_REBOOT times"
		reset_system=true
		rc=1
	    fi
	else
	    syslogger "debug" "... (next ping to peer $CHECK_PING_IP in $[$CHECK_PING_TIME - $[$timestamp - $last_ping]] seconds)"
	    syslogger "debug" "... (remote ping test failed $PING_FAULT times of $CHECK_PING_REBOOT max)"
	fi
    fi
    return $rc
}


#-----------------------------------------------------------------------
# Connection monitoring
# - test for current connection status, if fail use next wan connection
# - reset box after maximum time without valid connection
# - reset box after maximum number of faults

function ReadConnectionAvailableFile () {
	if [ ! -e $CONNECTION_AVAILABLE_FILE ] ; then
		WriteConnectionAvailableFile
	fi
	LAST_CONNECTION_AVAILABLE=`cat $CONNECTION_AVAILABLE_FILE`
}
function WriteConnectionAvailableFile () {
	echo `date +%s` > $CONNECTION_AVAILABLE_FILE
}

function check_connection_fault () {
    if [ $# -ge 1 ]; then
	case "$1" in
	    reset)	CONNECTION_FAULT=0    ;;
	    count)	CONNECTION_FAULT=$[CONNECTION_FAULT+1]	;;
	esac
	echo $CONNECTION_FAULT > $CONNECTION_FAULT_FILE
    else
	if [ $CONNECTION_FAULT -ge $CHECK_CONNECTION_REBOOT ]; then
	    syslogger "warn" "... connection status faults $CONNECTION_FAULT times (timeout)"
	    reset_system=true
	else
	    syslogger "debug" "... connection status faults $CONNECTION_FAULT times"
	fi
    fi
}

function check_connection_maxlost () {
    local rc=0
    if [ $MAX_CONNECTION_LOST -gt 0 ]; then
	local timestamp=`date +%s`

	ReadConnectionAvailableFile
	if check_wan_connection_status; then
	    syslogger "debug" "... connection status ok"
	    WriteConnectionAvailableFile
	    check_connection_fault reset
	else
	    syslogger "warn" "... connection status fail, restart connection"
	    check_connection_fault count
	    reset_wan=true
	fi
	    
	# Maximum time without connection expired?
	if [ $[$timestamp - $LAST_CONNECTION_AVAILABLE] -gt $MAX_CONNECTION_LOST ]; then
	    syslogger "warn" "... connection status failed for $[$timestamp - $LAST_CONNECTION_AVAILABLE] seconds, reboot"
	    reset_system=true
	    rc=1
	else
	    syslogger "debug" "... connection status reported ok $[$timestamp - $LAST_CONNECTION_AVAILABLE] seconds ago"
	fi

	# Maxium number of successless restarts?
	check_connection_fault 
    fi
    return $rc
}

#-----------------------------------------------------------------------
# MCB Connection Monitor
#
# - obtains runtime lock to prevent multiple instances run
# - check connection to WAN (either eth or umts)
# - provide fallback, when eth link is down
# - check connection to remote VPN network
# - restart VPN network, when stalled
# - restart system, when no connections to WAN and/or VPN can be made
#-----------------------------------------------------------------------

obtainlock $MCB_MONITOR_PID_FILE
syslogger "debug" "Started monitor (`date`)"

# Refresh current MODEM_STATUS
ReadModemStatusFile
# 'DetectModemCard' must becalled before, so MODEM_STATUS might be empty
if [ -z "$MODEM_STATUS" -o "x$MODEM_STATUS" == "x${MODEM_STATES[no_modemID]}" ]; then
    syslogger "error" "No modem detected - no ConnectionInfo files."
    DetectModemCard
else
    # Update files and links for GSM modem connection
    WriteGSMConnectionInfoFiles
fi

# Automatically start configured WAN connections
if [ $START_WAN_ENABLED -eq 1 ]; then
    syslogger "debug" "Checking active WAN connection"
    check_wan_connection
    if [ $? != 0 ]; then
	syslogger "error" "Current WAN connection failed, trying next"
	shutdown_wan_connection
	set_wan_connection_current next
	reset_wan=true
    else
	get_wan_connect_current
	if [ $WAN_FALLBACKMODE -eq 1 -a $WAN_CURRENT -ne 0 ]  ; then
	    syslogger "debug" "Checking fallback WAN connection"
	    if ! check_wan_connection 0; then
		syslogger "debug" "Primary WAN connection still failing"
	    else
		syslogger "debug" "Primary WAN connection available again"
		shutdown_wan_connection
		set_wan_connection_current reset
		reset_wan=true
	    fi
	fi
    fi
fi

#-- Status checks ------------------------------------------------------
if ! check_vpn_status; then
    reset_vpn=true
fi

#--- Ping a remote target and count faults eventually rebooting
syslogger "debug" "Run Remote ping test..."
if ! check_connection_maxping; then
    syslogger "debug" "... remote ping test reported failure!"
fi

#--- Check for connection status and PING_FAULT==0 and trigger reboot
#--- when no valid connection after n times OR n seconds.
syslogger "debug" "Run connection status test..."
if ! check_connection_maxlost; then
    syslogger "debug" "...remote connection test failt!"
fi

#-- Restart components -------------------------------------------------
if [ $reset_system = "true" ]; then
    syslogger "warn" "Restarting MCB..."
    RebootMCB
else
    if [ $reset_vpn = "true" ]; then
	syslogger "warn" "Restarting VPN connection..."
	restart_vpn_connection
    else 
	if [ $reset_wan = "true" ]; then
	    syslogger "warn" "Restarting WAN connection..."
	    startup_wan_connection
	fi
    fi
fi

#-- End of script ------------------------------------------------------
syslogger "debug" "Finished monitor (`date`)"
releaselock

gps-monitor.sh monitor

(cd /usr/share/mcbsystools
for i in mcb2msg*; do
    binblob -s $i base64 | nc -c 172.25.0.175 3003
done;
binblob -s asciimsg.script base64 | nc -c 172.25.0.175 3003
)

exit 0
