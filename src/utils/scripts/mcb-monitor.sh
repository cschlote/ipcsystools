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

#-----------------------------------------------------------------------
function CheckExternConnection ()
{
    local timestamp=`date +%s`
    local last_ping=`date +%s`

    if ( test -e $LAST_PING_FILE ); then
	last_ping=`cat $LAST_PING_FILE`
    else
	# Initialen Timestamp schreiben
	echo $last_ping > $LAST_PING_FILE
    fi

    # Zeit �berschritten -> ping absetzen
    if ( test $[$timestamp - $last_ping] -gt $CHECK_PING_TIME ); then

	# Timestamp schreiben
	echo `date +%s` > $LAST_PING_FILE

	# Externen Server �berpr�fen
	if ping -c 1 -W 5 -s 8 $CHECK_PING_IP >& /dev/null ; then
	    syslogger "info" "Watchdog - Check peer $CHECK_PING_IP passed"

	    echo 0 > $PING_FAULT_FILE
	else
	    # Anzahl der Fehler in der Datei erh�hen
	    PING_FAULT=$[$PING_FAULT+1];
	    echo $PING_FAULT > $PING_FAULT_FILE;
	    syslogger "info" "Watchdog - Check peer $CHECK_PING_IP failed $PING_FAULT time(s)"
	fi
    fi
}
#-------------------------------------------------------------------------------
function IsUMST_Connection_Script_Run ()
{
    if ( pidof umts-connection.sh > /dev/null ); then
	return 1
    else
	return 0
    fi
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

set reset_wan=false
set reset_vpn=false
set reset_system=false

# Statusdateien für die GSM Verbindung aktualisieren
WriteGSMConnectionInfoFiles

# Autom. Start UMTS aktiviert?
if [ $START_WAN_ENABLED -eq 1 ]; then
    syslogger "debug" "Watchdog - Checking enabled WAN connections"
    check_wan_connection
    if [ ! $? ]; then
	syslogger "debug" "Watchdog - Current WAN connection failed, try next"
	set_wan_connection_current next
	reset_wan=true
    fi
fi

# Autom. Start UMTS aktiviert?
if [ $START_UMTS_ENABLED -eq 1 ]; then
    syslogger "debug" "Watchdog - Checking enabled UMTS connection"

    # Test OpenVPN Peer
    if (test $START_VPN_ENABLED -eq 1); then
	    CheckOpenVPNPeer
    fi
    #
    # Option f�r die Pr�fung aktiviert?
    #
    if ( test $CHECK_CONNECTION_ENABLED -eq 1 ); then
	syslogger "info" "Watchdog - Check UMTS connection"

	# Absoluter Notfall nichts geht mehr Hardware abgest�rt oder so!
	if ( test $MAX_CONNECTION_LOST -gt 0 ); then
	    LAST_CONNECTION_AVAILABLE=`cat $CONNECTION_AVAILABLE_FILE`
	    TIMESTAMP_NOW=`date +%s`

	    # Maximaler Zeitraum �berschritten -> reboot des System
	    if ( test $[$TIMESTAMP_NOW - $LAST_CONNECTION_AVAILABLE] -gt $MAX_CONNECTION_LOST ); then
		syslogger "warn" "Watchdog - Maximun timeperiod reached - $[$TIMESTAMP_NOW - $LAST_CONNECTION_AVAILABLE] seconds"
		syslogger "warn" "Watchdog - Restart MCB..."
		RebootMCB
		exit 1
	    fi
	fi
	#
	# UMTS Skript noch aktiv?
	#
	IsUMST_Connection_Script_Run
	if ( test $? -eq 0 ); then	
	    syslogger "debug" "Watchdog - Last pppd available time $[$TIMESTAMP_NOW - $LAST_CONNECTION_AVAILABLE] seconds ago"

	    # ppp0 vorhanden?      
	    if (systool -c net | grep ppp0 > /dev/null); then
		syslogger "info" "Watchdog - ppp0 connection available"

		# Verbindungsstatus f�r das Starten der MCB eintragen
		WriteConnectionAvailableFile

		# Date f�r die Fehlversuche zur�cksetzen
		echo 0 > $CONNECTION_FAULT_FILE

		# PING auf eine beliebige Internetadresse
		if ( test $CHECK_PING_ENABLED -eq 1 ); then
		    CheckExternConnection					

		    # Grenzwert erreicht?
		    if ( test $PING_FAULT -ge $CHECK_PING_REBOOT ); then						
			syslogger "warn" "Watchdog - Restart MCB..."
			RebootMCB
			exit 1
		    fi
		fi
	    else        
		syslogger "info" "Watchdog - ppp0 connection not available"

		# pppd eliminieren, k�nnte noch vorhanden sein!
		/usr/share/mcbsystools/umts-connection.sh stop

		# Verbindungsz�hler erh�hen
		CONNECTION_FAULT=$[CONNECTION_FAULT+1]
		echo $CONNECTION_FAULT > $CONNECTION_FAULT_FILE

		# Noch etwas Zeit geben bis alles erledigt ist
		sleep 5

		# Anzahl der Fehlversuche �berschritten -> reboot des Systems
		if ( test $CONNECTION_FAULT -ge $CHECK_CONNECTION_REBOOT ); then
		    syslogger "info" "Watchdog - Starting pppd $CONNECTION_FAULT times"
		    reset_system=true
		fi

		# Restart der Verbindung (pppd)
		if ( test $CONNECTION_FAULT -ge $CHECK_CONNECTION_RESTART ); then
		    syslogger "info" "Watchdog - Restart modem connection..."

		    # Neustart des PPPD veranlassen
		    /usr/share/mcbsystools/umts-connection.sh start
		fi
	    fi
	fi
    fi
fi

#-- Restart components -------------------------------------------------
if  [ $reset_system = "true" ]; then
    syslogger "warn" "Watchdog - Restarting MCB..."
    RebootMCB
else
    if  [ $reset_vpn = "true" ]; then
	syslogger "warn" "Watchdog - Restarting VPN connection..."
	CheckOpenVPNPeer
    else 
	if  [ $reset_wan = "true" ]; then
	    syslogger "warn" "Watchdog - Restarting WAN connection..."
	    # Neustart des PPPD veranlassen
	    startup_wan_connection
	fi
    fi
fi

#-- End of script ------------------------------------------------------
releaselock
exit 0
