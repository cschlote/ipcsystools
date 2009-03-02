#!/bin/bash
#**********************************************************************************
#
#        FILE: umts-connection.sh
#
#       USAGE: umts-connection.sh start || stop
#
# DESCRIPTION: Script starts the UMTS Connection
#
#      AUTHOR: Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY: konzeptpark GmbH, 35633 Lahnau
#
#**********************************************************************************

PATH=/bin:/usr/bin:/sbin:/usr/sbin

# Basis Bibliothek für die MCB-2
. /usr/share/mcbsystools/mcblib.inc

# PID - File für das Skript
UMTS_CONNECTION_PID_FILE=/var/run/umts_connection.pid

#-------------------------------------------------------------------------------
# Überprüft ob der PPPD aktiv ist
function IsPPPDAlive ()
{
  if (pidof pppd > /dev/null) ; then
    return 1
  else
    return 0
  fi
}
#-------------------------------------------------------------------------------
# PPPD für UMTS starten
function StartPPPD ()
{
  log "PPPD starten..."

  # Device der UMTS Karte aktualisieren
  RefreshDatacardDevice  
  local device=$CONNECTION_DEVICE

  log "Device " $device
	
  pppd $device 460800 connect "/usr/sbin/chat -v -f /usr/share/mcbsystools/ppp-umts.chat" &

  #pppd call gsm885 &
}
#-------------------------------------------------------------------------------
# PPPD stoppen
function StopPPPD ()
{
  local pids=`pidof pppd`

  log "PPPD beenden..."

  # Kein PPPD aktiv
  if test -z "$pids"; then
    echo "$0: No pppd is running."
    return 0
  fi

  # Alle PPPD's killen!
  kill -TERM $pids > /dev/null

  # Alle pid's löschen
  if [ ! "$?" = "0" ]; then
    # Alle pid's löschen
    rm -f /var/run/ppp*.pid > /dev/null
  fi
}
#-------------------------------------------------------------------------------
# Wartet bis das ppp0 device verfügbar ist
function WaitForPPP0Device ()
{
  # Counter für die Durchläufe
  local count_timeout=0
  local count_timeout_max=12
  local sleeptime=5
  local reached_timeout=0

  while [ true ] ; do

    # ppp0 device prüfen        
    if (systool -c net | grep ppp0 > /dev/null); then    
      log "device ppp0 verfügbar"

    	# Nach dem UMTS Startskript Zeit aktualisieren
    	echo `date +%s` > $CONNECTION_AVAILABLE_FILE

      break
    fi

    # Timeout überprüfen
    if [ $count_timeout -ge $count_timeout_max ]; then
      reached_timeout=1
      break
    fi

    debuglog "Warte auf device ppp0..."

    sleep $sleeptime

    count_timeout=$[count_timeout+1]
  done

  if [ $reached_timeout -eq 1 ]; then
    return 0
  else
    return 1
  fi
}
#-------------------------------------------------------------------------------
# Abfragen ob eine PCMCIA Karte im Sockel steckt.
# Wenn keine Karte steckt ist eine Verbindung unmöglich.
# Deshalb wird sofort mit Error beendet.
function WaitForDataCard ()
{
  # Counter für die Durchläufe
  local count_timeout=0
  local count_timeout_max=12
  local sleeptime=5
  local reached_timeout=0

  # Prüfung, ob Karte vorhanden und durch das System erkannt
  while [ true ]; do
  
    # Es wurde eine Karte in dem PCMCIA Slot gefunden
    #if /sbin/cardctl status | grep "ready" >/dev/null; then
    #  debuglog "Datenkarte wurde gefunden"
      
      # Sierra Wireless Modems
      if lsusb -d 1199: > /dev/null; then      
        debuglog "Sierra Wireless Modem wurden von dem System erkannt"        
        echo "/dev/ttyUSB4" > $CONNECTION_DEVICE_FILE
        echo "/dev/ttyUSB3" > $COMMAND_DEVICE_FILE                      
        break
      fi

      # Option Modems
      if lsusb -d 0af0: > /dev/null; then
        debuglog "Option Modem wurden von dem System erkannt"
        echo "/dev/ttyUSB0" > $CONNECTION_DEVICE_FILE
        echo "/dev/ttyUSB2" > $COMMAND_DEVICE_FILE
        break
      fi
      
    #fi

    sleep $sleeptime
  done

#  local pin_count=1
#  $UMTS_PIN $SIM_PIN
#  local pin_state=$?
#  while [ $pin_count -lt 10 ] && [ $pin_state -eq 254 ]
#  do
#    # Warten bis Karte eingebucht
#    sleep 1
#    $UMTS_PIN $SIM_PIN
#    pin_state=$?
#    pin_count=$[pin_count+1]
#  done

#  debuglog "pin_state in WaitForDataCard: " $pin_state;

#  local setop_state=0
#  if [ $OPERATOR_SELECTION -eq 0 ]; then
#    $UMTS_SETOP
#  else
#    $UMTS_SETOP $OPERATOR_ID
#  fi
#  local setop_state=$?
#
#  debuglog "setop_state in WaitForDataCard: " $setop_state;

  $UMTS_NI
  local ni_state=$?

  debuglog "ni_state in WaitForDataCard: " $ni_state;

  # Karte auf Verbindung (eingebucht) prüfen
  while [ $ni_state -ne 0 ]; do

    # Erhöhe die Anzahl der Versuche auf 18, falls Limited Service (d.h. ni_state==2) als Netz zurueckgegeben wird
    if [ $ni_state -eq 2 ]; then
      count_timeout_max=18
    fi

    # Timeout überprüfen
    if [ $count_timeout -ge $count_timeout_max ]; then
      reached_timeout=1
      break
    fi

    sleep $sleeptime

    # Ist die Karte eingebucht?
    $UMTS_NI
    ni_state=$?

    count_timeout=$[count_timeout+1]

    debuglog "Warte bis Datenkarte eingebucht: " $count_timeout " ni_state: " $ni_state
  done

  if [ $reached_timeout -eq 1 ]; then
    return 0 # Timeout wurde erreicht
  else
    return 1
  fi
}
#-------------------------------------------------------------------------------
# Die Karte setzt ein Lock-File im Filesystem. Dieses
# Lock sichert die einmaligkeit des Zugriffes zu. Falls
# dieses Lock besteht muss es entfernt werden da ansonsten
# nicht auf die Karte zugegriffen werden kann.
function DeleteDataCardLock ()
{
  if [ -e /var/lock/LCK..ttyUSB0 ]; then
    rm -f /var/lock/LCK..ttyUSB0 > /dev/null

    sleep 20
  fi
}

#-------------------------------------------------------------------------------
#
# Hauptroutine
#
#-------------------------------------------------------------------------------

# ProzessID-Datei erzeugen
echo $$ > $UMTS_CONNECTION_PID_FILE

case "$1" in

  start)

    # LED 3g Timer blinken
    /usr/share/mcbsystools/leds.sh 3g timer

    # Ist die Datenkarte noch "gelockt"?
    # DeleteDataCardLock

    # Funktion wartet bis die Datenkarte vom System erkannt wurde
    WaitForDataCard
    if [ $? -eq 1 ]; then
      # Wegen dem Einbuchen in das UMTS-Netz warten
      sleep 5

      # Feldstärke ausgeben
      $UMTS_FS
      log "Prüfung der Feldstärke: " $?

      # pppd starten
      IsPPPDAlive
      if [ $? -eq 0 ]; then

        # Default Gateway entfernen wird beim Starten der ppp-Verbindung neu gesetzt
#        route del default > /dev/null

        StartPPPD

        # Warte bis ppp0 device vorhanden oder timeout!
        WaitForPPP0Device

        # Wenn PPP0 Device vorhanden -> OpenVPN starten
        if [ $? -eq 1 ]; then
          log "PPP0 Up..."
        else
          # 3g LED ausschalten
          /usr/share/mcbsystools/leds.sh 3g off
        fi

      fi
    else
      log "Datenkarte konnte nicht initialisiert werden (timeout)"

      # 3g LED ausschalten
      /usr/share/mcbsystools/leds.sh 3g off

      # Haben wir wirklich keine Verbindung oder liegt ein Fehler im Netz vor
      $UMTS_FS
      debuglog "Prüfung der Feldstärke: (timeout) " $?
    fi

    # Prozessdatei löschen
    rm -f $UMTS_CONNECTION_PID_FILE

    exit 0
  ;;

  stop)    

    # Alle PPPD's eliminieren
    StopPPPD

    # Prozessdatei löschen
    rm -f $UMTS_CONNECTION_PID_FILE

    exit 0
  ;;


  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;

# case
esac
