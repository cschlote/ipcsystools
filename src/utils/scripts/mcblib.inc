
# Basisverzeichnis für der UMTS Tools
MCB_TOOLS_DIR=/usr/bin

# Konfigurationsdatei
MCBCTL_CONFIG=/etc/mcbctl.conf

# Basisverzeichnis für die Statusdateien
MCB_STATUSFILE_DIR=/var/run

# Log file für die Verbindung
CONNECTION_LOG_FILE=/var/log/connection.log

#-------------------------------------------------------------------------------
# Auslesen der Parameter aus mcbctl.conf

# Sektion [GENERAL]
START_UMTS_ENABLED=`cat $MCBCTL_CONFIG | grep general.start_umts_enabled | cut -d"=" -f2`
START_VPN_ENABLED=`cat $MCBCTL_CONFIG | grep general.start_vpn_enabled | cut -d"=" -f2`
LOG_LEVEL=`cat $MCBCTL_CONFIG | grep general.log_level | cut -d"=" -f2`

# Sektion [WATCHDOG-UMTS]
CHECK_CONNECTION_RESTART=`cat $MCBCTL_CONFIG | grep watchdog.3g.check_connection_restart | cut -d"=" -f2`
CHECK_CONNECTION_REBOOT=`cat $MCBCTL_CONFIG | grep watchdog.3g.check_connection_reboot | cut -d"=" -f2`
MAX_CONNECTION_LOST=`cat $MCBCTL_CONFIG | grep watchdog.3g.max_connection_lost | cut -d"=" -f2`

# Sektion [WATCHDOG-PING]
CHECK_CONNECTION_ENABLED=`cat $MCBCTL_CONFIG | grep watchdog.ping.check_connection_enabled | cut -d"=" -f2`
CHECK_PING_ENABLED=`cat $MCBCTL_CONFIG | grep watchdog.ping.check_ping_enabled | cut -d"=" -f2`
CHECK_PING_IP=`cat $MCBCTL_CONFIG | grep watchdog.ping.check_ping_ip | cut -d"=" -f2`
CHECK_PING_REBOOT=`cat $MCBCTL_CONFIG | grep watchdog.ping.check_ping_reboot | cut -d"=" -f2`
CHECK_PING_TIME=`cat $MCBCTL_CONFIG | grep watchdog.ping.check_ping_time | cut -d"=" -f2`

#-------------------------------------------------------------------------------
# Statusdateien der MCB

# Lokale Variablen für die Überwachung PPP Verbindung
CONNECTION_FAULT_FILE=$MCB_STATUSFILE_DIR/connection.fault
if [ ! -e $CONNECTION_FAULT_FILE ] ; then
  echo 0 > $CONNECTION_FAULT_FILE
fi
CONNECTION_FAULT=`cat $CONNECTION_FAULT_FILE`

# Speichert den Zeitpunkt wann die Verbindung das letzte Mal verfügbar war
CONNECTION_AVAILABLE_FILE=$MCB_STATUSFILE_DIR/connection_available
if [ ! -e $CONNECTION_AVAILABLE_FILE ] ; then
  # Letzte Verfügbarkeit bei ersten Zugriff auf die aktuelle Zeit setzen
  echo `date +%s` > $CONNECTION_AVAILABLE_FILE
fi

# Lokale Variablen für die Überwachung externe Verbindung
PING_FAULT_FILE=$MCB_STATUSFILE_DIR/ping.fault
if [ ! -e $PING_FAULT_FILE ] ; then
  echo 0 > $PING_FAULT_FILE
fi
PING_FAULT=`cat $PING_FAULT_FILE`

# Lokale Variablen fuer die Devices des internen Modems
COMMAND_DEVICE_FILE=/var/run/command_dev
CONNECTION_DEVICE_FILE=/var/run/connection_dev

# Zeitpunkt des letzten abgesetzten "ping"
LAST_PING_FILE=$MCB_STATUSFILE_DIR/last_ping_time

# Programm zur Netzidentifizierung
UMTS_NI="$MCB_TOOLS_DIR/umtscardtool -i"

# Programm für das Messen der Feldstärke UMTS
UMTS_FS="$MCB_TOOLS_DIR/umtscardtool -f"

# Programm zur Pin-Eingabe
UMTS_PIN="$MCB_TOOLS_DIR/pin"

# Programm zum Setzen des Operators
UMTS_SETOP="$MCB_TOOLS_DIR/setop"


#-------------------------------------------------------------------------------
#
# Globale Funktionen
#

#-------------------------------------------------------------------------------
# Log Funktion
function log ()
{
  if ( test $LOG_LEVEL -ge 1 ); then

    if ( test $LOG_LEVEL -eq 2 ); then
      logger -p local0.info -t checkconnection $@
    fi

    echo `date` "  ;checkconnection; " $@ >> $CONNECTION_LOG_FILE
  fi
}

#-------------------------------------------------------------------------------
# Debug Funktion
function debuglog ()
{
  if ( test $LOG_LEVEL -eq 2 ); then
    logger -p local0.debug -t checkconnection $@
    echo `date` "  ;checkconnection; " $@ >> $CONNECTION_LOG_FILE
  fi
}
#-------------------------------------------------------------------------------
function RebootMCB ()
{
  log "MCB wird neu gestartet."  

  # Reboot ausführen
  /sbin/reboot

  sleep 5
}
#-------------------------------------------------------------------------------
function RefreshDatacardDevice ()
{
  CONNECTION_DEVICE=`cat $CONNECTION_DEVICE_FILE`
}
#-------------------------------------------------------------------------------
