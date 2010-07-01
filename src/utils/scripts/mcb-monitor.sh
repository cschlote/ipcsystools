#!/bin/bash
#
# DESCRIPTION: Monitor the WAN and VPN Connections
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

obtainlock /var/lock/mcb-monitor.lock

#-----------------------------------------------------------------------
# Read variables from config file
#-----------------------------------------------------------------------

# Sektion [WATCHDOG-UMTS]
CHECK_CONNECTION_RESTART=`getmcboption watchdog.3g.check_connection_restart`
CHECK_CONNECTION_REBOOT=`getmcboption watchdog.3g.check_connection_reboot`
MAX_CONNECTION_LOST=`getmcboption watchdog.3g.max_connection_lost`

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
    if ( test $CHECK_PING_ENABLED -eq 1 ); then
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
		syslogger "debug" "Watchdog - Check ping peer $CHECK_PING_IP passed"
		PING_FAULT=0
	    else
		syslogger "info"  "Watchdog - Check ping peer $CHECK_PING_IP failed $PING_FAULT time(s)"
		PING_FAULT=$[$PING_FAULT+1]
	    fi
	    echo $PING_FAULT > $PING_FAULT_FILE
	else
	    syslogger "debug" "Watchdog - Next ping to peer $CHECK_PING_IP in $[$CHECK_PING_TIME - $[$timestamp - $last_ping]] seconds"
	fi

	# Maximum number of pings sent?
	if [ $PING_FAULT -ge $CHECK_PING_REBOOT ]; then
	    syslogger "error" "Watchdog - Check ping peer $CHECK_PING_IP expired"
	    PING_FAULT=0 ; echo $PING_FAULT > $PING_FAULT_FILE
	    reset_system=true
	    rc=1
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
	    syslogger "warn" "Watchdog - Restarted connections $CONNECTION_FAULT times (timeout)"
	    reset_system=true
	else
	    syslogger "info" "Watchdog - Restarted connections $CONNECTION_FAULT times"
	fi
    fi
}

function check_connection_maxlost () {
    local rc=0
    if [ $MAX_CONNECTION_LOST -gt 0 ]; then
	local timestamp=`date +%s`

	ReadConnectionAvailableFile
	if check_wan_connection_status; then
	    WriteConnectionAvailableFile
	    check_connection_fault reset
	else
	    check_connection_fault count
	    reset_wan=true
	fi
	    
	# Maximum time without connection expired?
	if [ $[$timestamp - $LAST_CONNECTION_AVAILABLE] -gt $MAX_CONNECTION_LOST ]; then
	    syslogger "warn" "Watchdog - Maximun timeperiod reached - $[$timestamp - $LAST_CONNECTION_AVAILABLE] seconds"
	    reset_system=true
	    rc=1
	else
	    syslogger "debug" "Watchdog - Last connection available time $[$timestamp - $LAST_CONNECTION_AVAILABLE] seconds ago"
	fi

	# Maxium number of successless restarts?
	check_connection_fault || rc=1
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

# Statusdateien für die GSM Verbindung aktualisieren
WriteGSMConnectionInfoFiles

# Automatically start configured WAN connections
if [ $START_WAN_ENABLED -eq 1 ]; then
    syslogger "debug" "Watchdog - Checking enabled WAN connections"
    check_wan_connection
    if [ $? != 0 ]; then
	syslogger "debug" "Watchdog - Current WAN connection failed, trying next"
	shutdown_wan_connection
	set_wan_connection_current next
	reset_wan=true
    else
	get_wan_connect_current
	if [ $WAN_FALLBACKMODE -eq 1 -a $WAN_CURRENT -ne 0 ]  ; then
	    check_wan_connection 0
	    if [ $? != 0 ]; then
		syslogger "debug" "Watchdog - Primary WAN connection still failing"
	    else
		syslogger "debug" "Watchdog - Primary WAN connection available again"
		shutdown_wan_connection
		set_wan_connection_current reset
		reset_wan=true
	    fi
	fi
    fi
fi

#-- Status checks ------------------------------------------------------
check_openvpn_status

check_connection_maxlost &&
    check_connection_maxping

#-- Restart components -------------------------------------------------
if [ $reset_system = "true" ]; then
    syslogger "warn" "Watchdog - Restarting MCB..."
    RebootMCB
else
    if [ $reset_vpn = "true" ]; then
	syslogger "warn" "Watchdog - Restarting VPN connection..."
	startup_vpn_connection
    else 
	if [ $reset_wan = "true" ]; then
	    syslogger "warn" "Watchdog - Restarting WAN connection..."
	    startup_wan_connection
	fi
    fi
fi

#-- End of script ------------------------------------------------------
releaselock
exit 0
