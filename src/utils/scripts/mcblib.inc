#!/bin/bash -u -x
#
# DESCRIPTION: Include file for bash scripts
#

# Base directory of UMTS tools
MCB_TOOLS_DIR=/usr/bin

# Base directory of mcbsystools scripts
MCB_SCRIPTS_DIR=/usr/share/mcbsystools

# Configuration file
MCBCTL_CONFIG=/etc/mcbctl.conf

# Base directory for status files
MCB_STATUSFILE_DIR=/var/run

# Programm zur Netzidentifizierung
UMTS_NI="$MCB_TOOLS_DIR/umtscardtool -i"

# Programm f�r das Messen der Feldst�rke UMTS
UMTS_FS="$MCB_TOOLS_DIR/umtscardtool -f"

# Programm zur Pin-Eingabe
UMTS_PIN="$MCB_TOOLS_DIR/umtscardtool -p"

#TODO Programm zum Setzen des Operators (Manuelle Netzwahl)

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

# Define paths to persistent status variables 
COMMAND_DEVICE_FILE=$MCB_STATUSFILE_DIR/command_dev
CONNECTION_DEVICE_FILE=$MCB_STATUSFILE_DIR/connection_dev
MODEM_STATUS_FILE=$MCB_STATUSFILE_DIR/modem_status
CONNECTION_MODE_FILE=$MCB_STATUSFILE_DIR/connection_mode
CONNECTION_FS_FILE=$MCB_STATUSFILE_DIR/connection_fs
CONNECTION_GSMINFO_FILE=$MCB_STATUSFILE_DIR/connection_gsminfo
VPN_STATE_FILE=$MCB_STATUSFILE_DIR/vpn_status

WAN_STATE_FILE=$MCB_STATUSFILE_DIR/wan_status
if [ ! -e $WAN_STATE_FILE ] ; then
	echo 0 > $WAN_STATE_FILE
fi

# Connection monitoring, remember successful test time
CONNECTION_AVAILABLE_FILE=$MCB_STATUSFILE_DIR/connection_available
if [ ! -e $CONNECTION_AVAILABLE_FILE ] ; then
	# create file with current date, if not available yet (after boot)
	echo `date +%s` > $CONNECTION_AVAILABLE_FILE
fi

#-----------------------------------------------------------------------
# Configuration file
#-----------------------------------------------------------------------
function getmcboption () {
	echo `cat $MCBCTL_CONFIG | grep $1 | cut -d"=" -f2`
}

# Read GENERAL parameters from mcbctl.conf
START_WAN_ENABLED=`getmcboption general.start_wan_enabled`
START_UMTS_ENABLED=`getmcboption general.start_umts_enabled`
START_VPN_ENABLED=`getmcboption general.start_vpn_enabled`
LOG_LEVEL=`getmcboption general.log_level`
CHECK_CONNECTION_ENABLED=`getmcboption general.check_connection_enabled`
WAN_INTERFACES=`getmcboption general.wan.interfaces`
WAN_CHECKTARGETS=`getmcboption wan.checktargets`

#-----------------------------------------------------------------------
# Shell locking
#-----------------------------------------------------------------------
function obtainlock () {
	lastlockfile=$1
	if ( set -o noclobber; echo "$$" > "$lastlockfile") 2> /dev/null; 
	then
		trap 'rm -f "$lastlockfile"; exit $?' INT TERM EXIT
	else
	   echo "Failed to acquire lockfile: $lastlockfile." 
	   echo "Held by $(cat $lastlockfile)"
	   exit 0
	fi
}
function releaselock () {
   rm -f "$lastlockfile"
   trap - INT TERM EXIT
}
   
#-----------------------------------------------------------------------
# Log Funktion
#-----------------------------------------------------------------------
# void syslogger (char *logtype, char *msg)
function syslogger ()
{
	DESC="mcb-monitor"
	# LogLevel 1
	if [ "$1" != "debug" ]; then
		logger -p local0.$1 -t $DESC $2
	else
		# LogLevel 2 (Debugging)
		if ( test $LOG_LEVEL -gt 1 ); then
			logger -p local0.$1 -t $DESC $2	
		fi
	fi
}

#-----------------------------------------------------------------------
# Modem Handling
#-----------------------------------------------------------------------

function RefreshModemDevices () {
	COMMAND_DEVICE=`cat $COMMAND_DEVICE_FILE`  
	CONNECTION_DEVICE=`cat $CONNECTION_DEVICE_FILE`
}

function WriteConnectionAvailableFile () {
	echo `date +%s` > $CONNECTION_AVAILABLE_FILE
}

# Write the radio access technology (RAT) mode to a file
function WriteConnectionRATModeFile () {
	local connectionmode=`$MCB_TOOLS_DIR/umtscardtool -s AT!GETRAT? | grep !GETRAT: | awk -F": " '{print $2}'`
	echo $connectionmode > $CONNECTION_MODE_FILE
}

# Write the current network mode to a file
function WriteConnectionNetworkModeFile () {
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

# Write the UMTS Field strength to a file and update the LEDBAR
function WriteConnectionFieldStrengthFile ()
{
	$UMTS_FS
	field_strength=$?

	echo $field_strength > $CONNECTION_FS_FILE
	/usr/share/mcbsystools/leds.sh gsmfs $field_strength
}

function WriteToModemStatusFile () {
	MODEM_STATUS=$1
	echo $MODEM_STATUS > $MODEM_STATUS_FILE
}

function WriteGSMCellInfoFile () {
	# Write Mobile Network Code
	local gsminfo=`$MCB_TOOLS_DIR/umtscardtool -s AT!GSMINFO? | egrep "(PLMN:|LAC:|Cell ID:).*"`	
	echo "$gsminfo" > $CONNECTION_GSMINFO_FILE

	# Write roaming info 
	local roaming=`$MCB_TOOLS_DIR/umtscardtool -s AT^SYSINFO | grep SYSINFO | awk -F"," '{print $3}'`
	echo -e "ROAMING:\t$roaming" >> $CONNECTION_GSMINFO_FILE
}

function WriteGSMConnectionInfoFiles () {
	# Update field strength
	WriteConnectionFieldStrengthFile

	# Write Network Mode
	WriteConnectionNetworkModeFile

	# Write GSM Information
	WriteGSMCellInfoFile
}

#-----------------------------------------------------------------------

function DetectModemCard () {
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
#-----------------------------------------------------------------------
# Sets the PIN for the SIM card
function SetSIMPIN ()
{
	local sim_pin=`getmcboption sim.pin`

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

#-----------------------------------------------------------------------
# Ist die Karte eingebucht?
function CheckNIState () {
	$UMTS_NI
	local ni=$?
	if [ $ni -eq 0 ]; then
		WriteToModemStatusFile ${MODEM_STATES[registeredID]}
		WriteGSMConnectionInfoFiles
	fi
	return $ni
}

function ReadModemStatus () {
	MODEM_STATUS=`cat $MODEM_STATUS_FILE`
}

function SetConnectionRAT () {	
	ratcode=`getmcboption modem.connectionmode`
	RefreshModemDevices
	$MCB_TOOLS_DIR/umtscardtool -d $COMMAND_DEVICE -s AT!SELRAT=$ratcode >& /dev/null	
}

function InitializeModem () {
	DetectModemCard
	if [ $? -eq 0 ]; then
		SetConnectionRAT
		SetSIMPIN
		sleep 1
		WriteConnectionFieldStrengthFile
	fi
}

#-------------------------------------------------------------------------------
# WAN related functions
#-------------------------------------------------------------------------------
function set_wan_connection_current () {
	local wan_curr=`cat $WAN_STATE_FILE`
	case $1 in
	reset)	wan_curr=0 ;;
	next)	wan_curr=$[$wan_curr +1]
			if [ $wan_curr >= ${#WAN_INTERFACES[@]} ]; then
				$wan_curr=0;
			fi
	esac
	echo $wan_curr > $WAN_STATE_FILE
}
function startup_wan_connection () {
	if [ $START_WAN_ENABLED -eq 1 ]; then
		local wan_curr=`cat $WAN_STATE_FILE`
		local wan_if=${WAN_INTERFACES[$wan_curr]}
		local wan_ct=${WAN_CHECKTARGETS[$wan_curr]}
		syslogger "debug" "Starting WAN connection $wan_if"
		$MCB_SCRIPTS_DIR/$wan_if-connection.sh start &
	
		# [ $START_UMTS_ENABLED -eq 1 ] && $MCB_SCRIPTS_DIR/umts-connection.sh start &
	fi
}
function check_wan_connection () {
	if [ $START_WAN_ENABLED -eq 1 ]; then
		local wan_curr=`cat $WAN_STATE_FILE`
		local wan_if=${WAN_INTERFACES[$wan_curr]}
		local wan_ct=${WAN_CHECKTARGETS[$wan_curr]}
		
		$MCB_SCRIPTS_DIR/$wan_if-connection.sh check $wan_ct
		if [ ! $? ]; then
			syslogger "error" $DESC "WAN interface $wan_if or pin to $wan_ct failed"
			return 1;
		fi

		ping -I $wan_if -c 1 $wan_ct
		if [ ! $? ]; then
			syslogger "error" $DESC "Ping to $wan_ct on WAN interface $wan_if failed"
			return 1;
		fi
	fi
	return 0
}

#-------------------------------------------------------------------------------
# VPN related functions
#-------------------------------------------------------------------------------

function CheckOpenVPNPeer () {	
	local tunnelstate="down"
	local checkmode=`getmcboption ovpn.monitorpeers`

	case "$checkmode" in
		"ifconfig"|"")
		  #echo "get ip address from client.conf"
			if [ -e /etc/openvpn/client.conf ]; then
		  	openvpnIP=`grep 'ifconfig' /etc/openvpn/client.conf | awk '{print $3}'`
			fi
		  ;;
		"watchdog")
		  #echo "get ip address from watchdog settings"
		  openvpnIP=`getmcboption watchdog.ping.check_ping_ip`
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

#-----------------------------------------------------------------------
# Misc
#-----------------------------------------------------------------------

function RebootMCB () {
	syslogger "info" "MCB wird neu gestartet."
	/sbin/reboot
}

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

#
# Add mcb-monitor.sh to system cron table
#
function setup_checkconnection () {
	if [ $CHECK_CONNECTION_ENABLED -eq 1 ] ; then
		entry=`crontab -l 2>/dev/null | grep mcb-monitor.sh | wc -l` > /dev/null
		if [ $entry -eq 0 ]; then	
			syslogger "info" "Creating crontab entry for mcb-monitor.sh ..."
			crontab -l 2>/dev/null /tmp/crontab.dump
			echo "*/2 * * * * /usr/share/mcbsystools/mcb-monitor.sh" >> /tmp/crontab.dump
			crontab /tmp/crontab.dump
			rm -f /tmp/crontab.dump
		fi
	fi
}
#
# Ping a target and return status RC
#
function ping_target () {
	return 0
}
