#**********************************************************************************
#
#        FILE: mcblib.sh
#
# DESCRIPTION: Include file for bash scripts
#
#      AUTHOR: Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY: konzeptpark GmbH, 35633 Lahnau
#
#**********************************************************************************

# Basisverzeichnis f�r der UMTS Tools
MCB_TOOLS_DIR=/usr/bin

# Konfigurationsdatei
MCBCTL_CONFIG=/etc/mcbctl.conf

# Basisverzeichnis f�r die Statusdateien
MCB_STATUSFILE_DIR=/var/run

# Log file f�r die Verbindung
CONNECTION_LOG_FILE=/var/log/connection.log

# Globale Variablen fuer die Devices des internen Modems
COMMAND_DEVICE_FILE=/var/run/command_dev
CONNECTION_DEVICE_FILE=/var/run/connection_dev
MODEM_STATUS_FILE=/var/run/modem_status

# Status for the modem
MODEM_STATES=( no_modem detected sim_not_inserted sim_pin ready registered connected )
# IDs for the modem state
no_modemID=0
detectedID=1
sim_not_insertedID=2
sim_pinID=3
readyID=4
registeredID=5
connected=6

# Programm zur Netzidentifizierung
UMTS_NI="$MCB_TOOLS_DIR/umtscardtool -i"

# Programm f�r das Messen der Feldst�rke UMTS
UMTS_FS="$MCB_TOOLS_DIR/umtscardtool -f"

# Programm zur Pin-Eingabe
UMTS_PIN="$MCB_TOOLS_DIR/umtscardtool -p"

#TODO Programm zum Setzen des Operators


#-------------------------------------------------------------------------------
# Auslesen der GENERAL-Parameter aus mcbctl.conf

# Sektion [GENERAL]
START_UMTS_ENABLED=`cat $MCBCTL_CONFIG | grep general.start_umts_enabled | cut -d"=" -f2`
START_VPN_ENABLED=`cat $MCBCTL_CONFIG | grep general.start_vpn_enabled | cut -d"=" -f2`
LOG_LEVEL=`cat $MCBCTL_CONFIG | grep general.log_level | cut -d"=" -f2`
CHECK_CONNECTION_ENABLED=`cat $MCBCTL_CONFIG | grep general.check_connection_enabled | cut -d"=" -f2`

#-------------------------------------------------------------------------------
# Speichert den Zeitpunkt wann die Verbindung das letzte Mal verf�gbar war	
CONNECTION_AVAILABLE_FILE=$MCB_STATUSFILE_DIR/connection_available
if [ ! -e $CONNECTION_AVAILABLE_FILE ] ; then
	# Letzte Verf�gbarkeit bei ersten Zugriff auf die aktuelle Zeit setzen
	echo `date +%s` > $CONNECTION_AVAILABLE_FILE
fi

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

  # Reboot ausf�hren
  /sbin/reboot
}
#-------------------------------------------------------------------------------
function RefreshModemDevices ()
{
	COMMAND_DEVICE=`cat $COMMAND_DEVICE_FILE`  
	CONNECTION_DEVICE=`cat $CONNECTION_DEVICE_FILE`
}
#-------------------------------------------------------------------------------
function WriteConnectionAvailableFile ()
{
	# Letzte Verf�gbarkeit bei ersten Zugriff auf die aktuelle Zeit setzen
	echo `date +%s` > $CONNECTION_AVAILABLE_FILE
}
#-------------------------------------------------------------------------------
# Write the radio access technology (RAT) mode to a file
function WriteConnectionRATModeFile ()
{
	local connectionmode=`$MCB_TOOLS_DIR/umtscardtool -s AT!GETRAT? | grep !GETRAT: | awk -F": " '{print $2}'`
	echo $connectionmode > /var/run/connection_mode
}
#-------------------------------------------------------------------------------
# Write the current network mode to a file
function WriteConnectionNetworkModeFile ()
{
	local connectionmode
	local cnti=`umtscardtool -s AT*CNTI=0 | grep *CNTI:`
	
	# *CNTI: +CME ERROR: no network service
	local err=`expr index "$cnti" "ERROR"`
	if [ $err -eq 0 ]; then
		connectionmode=`echo $cnti | awk -F"," '{print $2}'`	
	else
		connectionmode=`echo $cnti | awk -F":" '{print $3}'`		
	fi
	echo $connectionmode > /var/run/connection_mode
}
#-------------------------------------------------------------------------------
# Write the UMTS Field strength to a file and update the LEDBAR
function WriteConnectionFieldStrengthFile ()
{
	# Feldst�rke auf der MCB aktualisieren
  $UMTS_FS
  field_strength=$?

	# Feldst�rke Datei aktualisieren
	echo $field_strength > /var/run/connection_fs

	# LED's aktualisieren
  /usr/share/mcbsystools/leds.sh gsmfs $field_strength
}
#-------------------------------------------------------------------------------
function WriteToModemStatusFile ()
{
	MODEM_STATUS=$1
	echo $MODEM_STATUS > $MODEM_STATUS_FILE
}
#-------------------------------------------------------------------------------
# Detect the Modemcard and write the device
function DetectModemCard ()
{
	# Sierra Wireless Modems
	if lsusb -d 1199: > /dev/null; then
		debuglog "Sierra Wireless Modem wurden von dem System erkannt"
		echo "/dev/ttyUSB4" > $CONNECTION_DEVICE_FILE
		echo "/dev/ttyUSB3" > $COMMAND_DEVICE_FILE

		WriteToModemStatusFile ${MODEM_STATES[detectedID]}
		return 0
	else
		WriteToModemStatusFile ${MODEM_STATES[no_modemID]}
		return 1
	fi
}
#-------------------------------------------------------------------------------
# Sets the PIN for the SIM card
function SetSIMPIN ()
{
	local sim_pin=`grep 'sim.pin' /etc/mcbctl.conf | cut -d"=" -f2`

	$UMTS_PIN $sim_pin
	local pin_state=$?

	#  1: PIN angegeben, musste aber nicht gesetzt werden
	#  2: SIM Karte wurde nicht erkannt
	#  3: Der PIN wird ben�tigt, wurde aber nicht angegeben
	#  4: PUK oder SuperPIN ben�tigt. SIM-Karte entnehmen und mit einem Mobiltelefon entsperren.
	#  5: Die eingegebene PIN war falsch.
	#  6: Der AT-Befehl zum Setzen der PIN hat einen Fehler erzeugt.
	case $pin_state in
		0|1)
			WriteToModemStatusFile ${MODEM_STATES[readyID]} ;;
		2)
			WriteToModemStatusFile ${MODEM_STATES[sim_not_insertedID]} ;;
		3)
			WriteToModemStatusFile ${MODEM_STATES[sim_pinID]} ;;
		*)
			WriteToModemStatusFile "errorcode: $pin_state" ;;
	esac
}
#-------------------------------------------------------------------------------
# Ist die Karte eingebucht?
function CheckNIState ()
{
  $UMTS_NI
	local ni=$?
	if [ $ni -eq 0 ]; then
		WriteToModemStatusFile ${MODEM_STATES[registeredID]}
	fi
  return $ni
}
#-------------------------------------------------------------------------------
function ReadModemStatus ()
{
	MODEM_STATUS=`cat $MODEM_STATUS_FILE`
}
#-------------------------------------------------------------------------------
# Sets current radio access technology (RAT)
function SetConnectionRAT ()
{	
	ratcode=`cat $MCBCTL_CONFIG | grep modem.connectionmode | cut -d"=" -f2`
	RefreshModemDevices
	$MCB_TOOLS_DIR/umtscardtool -d $COMMAND_DEVICE -s AT!SELRAT=$ratcode	
}
#-------------------------------------------------------------------------------
function InitializeModem ()
{
	DetectModemCard
	if [ $? -eq 0 ]; then
		SetConnectionRAT		
		SetSIMPIN
		sleep 1
		WriteConnectionFieldStrengthFile
	fi
}
#-------------------------------------------------------------------------------
function CheckOpenVPNPeer ()
{
	openvpnIP=`grep 'ifconfig' /etc/openvpn/client.conf | awk '{print $3}'`	
	if ping -c 1 -W 5 -s 8 $openvpnIP >& /dev/null ; then
		#echo `date` "  ;Externer Rechner ($openvpnIP) wurde erreicht"
		/usr/share/mcbsystools/leds.sh vpn on
	else
		/usr/share/mcbsystools/leds.sh vpn off
	fi	
}
#-------------------------------------------------------------------------------
