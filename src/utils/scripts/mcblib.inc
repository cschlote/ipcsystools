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

# Globale Variablen fuer die Devices des internen Modems
COMMAND_DEVICE_FILE=$MCB_STATUSFILE_DIR/command_dev
CONNECTION_DEVICE_FILE=$MCB_STATUSFILE_DIR/connection_dev
MODEM_STATUS_FILE=$MCB_STATUSFILE_DIR/modem_status
CONNECTION_MODE_FILE=$MCB_STATUSFILE_DIR/connection_mode
CONNECTION_FS_FILE=$MCB_STATUSFILE_DIR/connection_fs
CONNECTION_GSMINFO_FILE=$MCB_STATUSFILE_DIR/connection_gsminfo
VPN_STATE_FILE=$MCB_STATUSFILE_DIR/vpn_status

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

#TODO Programm zum Setzen des Operators (Manuelle Netzwahl)


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
# void syslogger (char *logtype, char *msg)
function syslogger ()
{
	DESC="mcb-monitor"

	# LogLevel 1
	if ( test "$1" != "debug" ); then
		logger -p local0.$1 -t $DESC $2
	else
		# LogLevel 2 (Debugging)
		if ( test $LOG_LEVEL -gt 1 ); then
			logger -p local0.$1 -t $DESC $2	
		fi
	fi
}
#-------------------------------------------------------------------------------
# Reboot the system
function RebootMCB ()
{  
	/sbin/reboot
}
#-------------------------------------------------------------------------------
# Validate an IP Address
# void valid_ip (char *ipaddress)
function valid_ip ()
{
	local  ip=$1
	local  stat=1

	 if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
		 OIFS=$IFS
		 IFS='.'
		 ip=($ip)
		 IFS=$OIFS
		 [[ ${ip[0]} -le 255 && ${ip[1]} -le 255 && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
		 stat=$?
	 fi
	 return $stat
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
	echo $connectionmode > $CONNECTION_MODE_FILE
}
#-------------------------------------------------------------------------------
# Write the current network mode to a file
function WriteConnectionNetworkModeFile ()
{
	local connectionmode
	local cnti=`umtscardtool -s AT*CNTI=0 | grep *CNTI:`
	
	# *CNTI: +CME ERROR: no network service
	local err=`expr match "$cnti" ".*\ERROR"`
	if [ $err -eq 0 ]; then
		connectionmode=`echo $cnti | awk -F"," '{print $2}'`	
	else
		connectionmode=`echo $cnti | awk -F":" '{print $3}'`		
	fi
	echo $connectionmode > $CONNECTION_MODE_FILE
}
#-------------------------------------------------------------------------------
# Write the UMTS Field strength to a file and update the LEDBAR
function WriteConnectionFieldStrengthFile ()
{
	# Feldst�rke auf der MCB aktualisieren
  $UMTS_FS
  field_strength=$?

	# Feldst�rke Datei aktualisieren
	echo $field_strength > $CONNECTION_FS_FILE

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
function WriteGSMCellInfoFile ()
{
	# Mobile Network Code schreiben	
	local gsminfo=`$MCB_TOOLS_DIR/umtscardtool -s AT!GSMINFO? | egrep "(PLMN:|LAC:|Cell ID:).*"`	
	echo "$gsminfo" > $CONNECTION_GSMINFO_FILE

	# Roaming schreiben
	local roaming=`$MCB_TOOLS_DIR/umtscardtool -s AT^SYSINFO | grep SYSINFO | awk -F"," '{print $3}'`
	echo -e "ROAMING:\t$roaming" >> $CONNECTION_GSMINFO_FILE
}
#-------------------------------------------------------------------------------
function WriteGSMConnectionInfoFiles ()
{
	# Update field strength
	WriteConnectionFieldStrengthFile

	# Write Network Mode
	WriteConnectionNetworkModeFile

	# Write GSM Information
	WriteGSMCellInfoFile
}
#-------------------------------------------------------------------------------
# Detect the Modemcard and write the device
function DetectModemCard ()
{
	# Sierra Wireless Modems
	if lsusb -d 1199: > /dev/null; then
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
		WriteGSMConnectionInfoFiles
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
	$MCB_TOOLS_DIR/umtscardtool -d $COMMAND_DEVICE -s AT!SELRAT=$ratcode >& /dev/null	
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
	local tunnelstate="down"
	local checkmode=`grep 'ovpn.monitorpeers' /etc/mcbctl.conf | cut -d "=" -f2`

	case "$checkmode" in
		"ifconfig"|"")
		  #echo "get ip address from client.conf"
			if [ -e /etc/openvpn/client.conf ]; then
		  	openvpnIP=`grep 'ifconfig' /etc/openvpn/client.conf | awk '{print $3}'`
			fi
		  ;;
		"watchdog")
		  #echo "get ip address from watchdog settings"
		  openvpnIP=`grep 'watchdog.ping.check_ping_ip' /etc/mcbctl.conf | cut -d"=" -f2`
		  ;;
		*)
		  #echo "validate ip address"
		  openvpnIP=$checkmode
		  ;;
	esac

	# Ping the peer address
	if (valid_ip $openvpnIP); then
		if ping -c 1 -W 5 -s 8 $openvpnIP >& /dev/null ; then
			/usr/share/mcbsystools/leds.sh vpn on
			tunnelstate="up"
		else
			/usr/share/mcbsystools/leds.sh vpn off
			tunnelstate="down"
		fi
	else 
		syslogger "warn" "Wrong OpenVPN monitor peer settings ($openvpnIP)"
		tunnelstate="down"
	fi

	# Set Tunnel state
	echo $tunnelstate > $VPN_STATE_FILE
	syslogger "info" "OpenVPN monitor peer ($openvpnIP) tunnel $tunnelstate"
}
#-------------------------------------------------------------------------------

