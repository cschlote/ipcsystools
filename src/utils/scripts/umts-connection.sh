#! /bin/sh
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

# Basis Bibliothek fuer die MCB-2
. /usr/share/mcbsystools/mcblib.inc

# PID - File fuer das Skript
UMTS_CONNECTION_PID_FILE=/var/run/umts_connection.pid

#-------------------------------------------------------------------------------
# ueberprueft ob der PPPD aktiv ist
function IsPPPDAlive ()
{
  if (pidof pppd > /dev/null) ; then
    return 1
  else
    return 0
  fi
}
#-------------------------------------------------------------------------------
# PPPD fuer UMTS starten
function StartPPPD ()
{
  log "PPPD starten..."

  # Device der UMTS Karte aktualisieren
  RefreshDatacardDevice
  local device=$CONNECTION_DEVICE
  pppd $device 460800 connect "/usr/sbin/chat -v -f /usr/share/mcbsystools/ppp-umts.chat" &
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

  # Alle pid's lueschen
  if [ ! "$?" = "0" ]; then
    # Alle pid's lueschen
    rm -f /var/run/ppp*.pid > /dev/null
  fi
}
#-------------------------------------------------------------------------------
# Wartet bis das ppp0 device verfuegbar ist
function WaitForPPP0Device ()
{
  # Counter fuer die Durchlueufe
  local count_timeout=0
  local count_timeout_max=12
  local sleeptime=5
  local reached_timeout=0

  while [ true ] ; do

    # ppp0 device pruefen        
    if (systool -c net | grep ppp0 > /dev/null); then    
      log "device ppp0 verfuegbar"

    	# Nach dem UMTS Startskript Zeit aktualisieren
    	WriteConnectionAvailableFile

      break
    fi

    # Timeout ueberpruefen
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
# Wartet bis das Modem eingebucht ist
function WaitForDataCard ()
{
  # Counter fuer die Durchlueufe
  local count_timeout=0
  local count_timeout_max=12
  local sleeptime=5
  local reached_timeout=0
	  
	# Check SIM PIN
	SetSIMPIN

	#TODO: Set operator selection

	CheckNIState
  local ni_state=$?

  debuglog "ni_state in WaitForDataCard: " $ni_state;

  # Karte auf Verbindung (eingebucht) pruefen
  while [ $ni_state -ne 0 ]; do

    # Erhoehe die Anzahl der Versuche auf 18, falls Limited Service (d.h. ni_state==2) als Netz zurueckgegeben wird
    if [ $ni_state -eq 2 ]; then
      count_timeout_max=18
    fi

    # Timeout ueberpruefen
    if [ $count_timeout -ge $count_timeout_max ]; then
      reached_timeout=1
      break
    fi

    sleep $sleeptime

    # Ist die Karte eingebucht?
		CheckNIState
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
#
# Hauptroutine
#
#-------------------------------------------------------------------------------

# ProzessID-Datei erzeugen
echo $$ > $UMTS_CONNECTION_PID_FILE

case "$1" in

	start)
		# Status des Modem
		ReadModemStatus
		if ( [ $MODEM_STATUS == ${MODEM_STATES[detectedID]} ] || \
				 [ $MODEM_STATUS == ${MODEM_STATES[readyID]} ] || \
				 [ $MODEM_STATUS == ${MODEM_STATES[registeredID]} ] ); then

			# LED 3g Timer blinken
			/usr/share/mcbsystools/leds.sh 3g timer

			# Funktion wartet bis die Datenkarte vom System erkannt wurde und eingebucht ist
			WaitForDataCard
			if [ $? -eq 1 ]; then
			  # Wegen dem Einbuchen in das UMTS-Netz warten
			  sleep 1

			  # Feldstuerke ausgeben
				WriteConnectionFieldStrengthFile

				# Netzmode ausgeben GPRS...HSDPA
				WriteConnectionNetworkModeFile

			  # pppd starten
			  IsPPPDAlive
			  if [ $? -eq 0 ]; then			
					# Starts the pppd
			    StartPPPD

			    # Warte bis ppp0 device vorhanden oder timeout!
			    WaitForPPP0Device

			    # PPPD konnte nicht gestartet werden
					if [ $? -eq 1 ]; then		
						WriteToModemStatusFile ${MODEM_STATES[connected]}
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
			  debuglog "Pruefung der Feldstuerke: (timeout) " $?
			fi		  
		fi
	  exit 0
	;;

	stop)
	  # Alle PPPD's eliminieren
	  StopPPPD
		CheckNIState
	  exit 0
	;;

	*)
	  echo "Usage: $0 {start|stop}"
	  exit 1
	  ;;

# case
esac

# Prozessdatei lueschen
rm -f $UMTS_CONNECTION_PID_FILE


