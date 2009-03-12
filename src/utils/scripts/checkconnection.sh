#!/bin/bash
#**********************************************************************************
#
#        FILE: checkconnection.sh
#
#       USAGE: checkconnection.sh
#
# DESCRIPTION: Monitor the UMTS Connection
#
#      AUTHOR: Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY: konzeptpark GmbH, 35633 Lahnau
#
#**********************************************************************************

PATH=/bin:/usr/bin:/sbin:/usr/sbin

# Basis Bibliothek für die MCB-2
. /usr/share/mcbsystools/mcblib.inc

#-------------------------------------------------------------------------------
# Überprüft ob das Skript bereits aktiv ist
function IsSelfRunOnce ()
{
  # Anzahl der aktiven Prozesse
  run_pids=`ps ax | grep checkconnection.sh | grep -v grep -c`

  # Dekrementieren wegen Kindprozess
  run_pids=$[run_pids-1]

  if ( test $run_pids -eq 1 ) ; then
    return 1
  else
    return 0
  fi
}
#-------------------------------------------------------------------------------
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

  # Zeit überschritten -> ping absetzen
  if ( test $[$timestamp - $last_ping] -gt $CHECK_PING_TIME ); then

    # Timestamp schreiben
    echo `date +%s` > $LAST_PING_FILE

    # Externen Server überprüfen
    if ping -c 1 -W 5 -s 8 $CHECK_PING_IP >& /dev/null ; then

      if ( test $LOG_LEVEL -ge 1 ); then
        echo `date` "  ;Externer Rechner ($CHECK_PING_IP) wurde erreicht" $@ >> $CONNECTION_LOG_FILE
      fi

      echo 0 > $PING_FAULT_FILE
    else
      # Anzahl der Fehler in der Datei erhöhen
      PING_FAULT=$[$PING_FAULT+1];
      echo $PING_FAULT > $PING_FAULT_FILE;

      if ( test $LOG_LEVEL -ge 1 ); then
        echo `date` "  ;Rechner wurde nicht erreicht" $@ >> $CONNECTION_LOG_FILE
      fi
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
#-------------------------------------------------------------------------------
#
# Hauptroutine
#
#-------------------------------------------------------------------------------
# Skript muss exklusiv laufen
# IsSelfRunOnce integrieren??!!

# UMTS aktiviert?
if (test $START_UMTS_ENABLED -eq 1); then

  # Feldstärke auf der MCB aktualisieren	
  $UMTS_FS
  field_strength=$?
	if (test $field_strength -lt 32); then
    /usr/share/mcbsystools/leds.sh gsmfs $field_strength
	fi

  # Option für die Prüfung aktiviert?
  if ( test $CHECK_CONNECTION_ENABLED -eq 1 ); then

    # Absoluter Notfall nichts geht mehr Hardware abgestürt oder so!
    if ( test $MAX_CONNECTION_LOST -gt 0 ); then

      LAST_CONNECTION_AVAILABLE=`cat $CONNECTION_AVAILABLE_FILE`
      TIMESTAMP_NOW=`date +%s`

      debuglog "Letzter ppp0: " $[$TIMESTAMP_NOW - $LAST_CONNECTION_AVAILABLE]

      # Maximaler Zeitraum überschritten -> reboot des System
      if ( test $[$TIMESTAMP_NOW - $LAST_CONNECTION_AVAILABLE] -gt $MAX_CONNECTION_LOST ); then
        log "Maximaler Wartezeitraum erreicht: " $[$TIMESTAMP_NOW - $LAST_CONNECTION_AVAILABLE]
        RebootMCB
        exit 1
      fi
    fi

    # UMTS Skript noch aktiv?
    IsUMST_Connection_Script_Run
    if ( test $? -eq 0 ); then
	
      # ppp0 vorhanden?      
      if (systool -c net | grep ppp0 > /dev/null); then    
        debuglog "PPP Verbindung ok!"

        # Verbindungsstatus für das Starten der MCB eintragen
        echo `date +%s` > $CONNECTION_AVAILABLE_FILE

        # Date für die Fehlversuche zurücksetzen
        echo 0 > $CONNECTION_FAULT_FILE

        # PING auf eine beliebige Internetadresse
        if ( test $CHECK_PING_ENABLED -eq 1 ); then
          CheckExternConnection

          # Grenzwert erreicht?
          if ( test $PING_FAULT -ge $CHECK_PING_REBOOT ); then
            log "Externe Überwachung fehlgeschlagen (ping)"
            RebootMCB
            exit 1
          fi
        fi
      else
        debuglog "PPP Verbindung nicht ok!"

        # pppd eliminieren, könnte noch vorhanden sein!
				/usr/share/mcbsystools/umts-connection.sh stop

        # Verbindungszähler erhöhen
        CONNECTION_FAULT=$[CONNECTION_FAULT+1]
        echo $CONNECTION_FAULT > $CONNECTION_FAULT_FILE

        # Noch etwas Zeit geben bis alles erledigt ist
        sleep 5

        # Anzahl der Fehlversuche überschritten -> reboot des Systems
        if ( test $CONNECTION_FAULT -ge $CHECK_CONNECTION_REBOOT ); then
          RebootMCB
          exit 1
        fi

        # Restart der Verbindung (pppd)
        if ( test $CONNECTION_FAULT -ge $CHECK_CONNECTION_RESTART ); then
          log "UMTS Skript wird neu gestartet."

          # Neustart des PPPD veranlassen
          /usr/share/mcbsystools/umts-connection.sh start
        fi
      fi
    fi
  fi
fi
