#!/bin/bash
#
# DESCRIPTION: Include file for bash scripts
#

# Base directory of ipc tools
IPC_TOOLS_DIR=/usr/bin

# Base directory of IPC-Systools scripts
IPC_SCRIPTS_DIR=/usr/share/ipcsystools

# Configuration file
IPCCTL_CONFIG=/etc/ipcsystools.conf

# Base directory for status files
IPC_STATUSFILE_DIR=/var/run/ipcsystools

# Base directory for lock files
IPC_LOCKFILE_DIR=/var/lock/

# Programm zur Netzidentifizierung
UMTS_NI="$IPC_TOOLS_DIR/umtscardtool -i"

# Programm für das Messen der Feldstärke
UMTS_FS="$IPC_TOOLS_DIR/umtscardtool -f"

# Programm zur Pin-Eingabe
UMTS_PIN="$IPC_TOOLS_DIR/umtscardtool -p"

# Create path to status files
if ! [ -e $IPC_STATUSFILE_DIR ]; then
	mkdir -p $IPC_STATUSFILE_DIR
fi

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
COMMAND_DEVICE_FILE=$IPC_STATUSFILE_DIR/command_dev
CONNECTION_DEVICE_FILE=$IPC_STATUSFILE_DIR/connection_dev
MODEM_STATUS_FILE=$IPC_STATUSFILE_DIR/modem_status
CONNECTION_MODE_FILE=$IPC_STATUSFILE_DIR/connection_mode
CONNECTION_FS_FILE=$IPC_STATUSFILE_DIR/connection_fs
CONNECTION_GSMINFO_FILE=$IPC_STATUSFILE_DIR/connection_gsminfo
VPN_STATE_FILE=$IPC_STATUSFILE_DIR/vpn_status

WAN_STATE_FILE=$IPC_STATUSFILE_DIR/wan_status
if [ ! -e $WAN_STATE_FILE ] ; then
	echo 0 > $WAN_STATE_FILE
fi

# Connection monitoring, remember successful test time
CONNECTION_AVAILABLE_FILE=$IPC_STATUSFILE_DIR/connection_available

#-----------------------------------------------------------------------
# Configuration file
#-----------------------------------------------------------------------
function getipcoption () {
	echo `cat $IPCCTL_CONFIG | grep ^$1= | cut -d"=" -f2 | cut -d"#" -f1`
}

# Read GENERAL parameters from ipcsystools.conf
START_WAN_ENABLED=`getipcoption general.start_wan_enabled`
LOG_LEVEL=`getipcoption general.log_level`

WAN_FALLBACKMODE=`getipcoption general.wan.fallbackmode`
WAN_INTERFACES=(`getipcoption general.wan.interfaces`)
WAN_CHECKTARGETS=(`getipcoption general.wan.checktargets`)
WAN_CHECKGATEWAYS=(`getipcoption general.wan.checkgateways`)

# Evaluate parameter START_UMTS_ENABLED
if [ ${WAN_INTERFACES[0]} == "umts" ]; then
	START_UMTS_ENABLED=1
else
	START_UMTS_ENABLED=0
fi

#-----------------------------------------------------------------------
# Shell locking
#-----------------------------------------------------------------------
function obtainlock ()
{
	lastlockfile=$1
	if ( set -o noclobber; echo "$$" > "$lastlockfile") 2> /dev/null;
	then
		trap 'rm -f "$lastlockfile"; exit $?' INT TERM EXIT
	else
	   syslogger "error" "Failed to acquire lockfile: $lastlockfile."
	   syslogger "error" "Held by $(cat $lastlockfile)"
	   echo "ERROR: Can't get lock. Script instance already running" 1>&2
	   exit 0
	fi
}
function releaselock ()
{
   rm -f "$lastlockfile"
   trap - INT TERM EXIT
}

#-----------------------------------------------------------------------
# Log Funktion
#-----------------------------------------------------------------------
# void syslogger (char *logtype, char *msg)
function syslogger ()
{
	local logDESC=${DESC:=$0[$$]}
	# LogLevel =1 loging enabled, >1 includes debugging messages
	if [ "$1" != "debug" -a $LOG_LEVEL -ge 1 ] || [ "$1" = "debug" -a $LOG_LEVEL -gt 1 ]; then
		logger -p local0.$1 -t $DESC "$2"
	fi
}

#-----------------------------------------------------------------------
# Modem Handling
#-----------------------------------------------------------------------

function RefreshModemDevices ()
{
	COMMAND_DEVICE=`cat $COMMAND_DEVICE_FILE`
	CONNECTION_DEVICE=`cat $CONNECTION_DEVICE_FILE`
}

#-----------------------------------------------------------------------

# Write the UMTS Field strength to a file and update the LEDBAR
function WriteConnectionFieldStrengthFile ()
{
#	$UMTS_FS
	syslogger "debug" "FIXME Get fieldstreng via modemmanager"
	
	field_strength=$?

	echo $field_strength > $CONNECTION_FS_FILE
	ipc-set-led gsmfs $field_strength ||
		syslogger "warn" "ipc-set-led gsmfs return error"
}

function ResetModem ()
{
#	$IPC_TOOLS_DIR/umtscardtool -s ATZ >/dev/null
	syslogger "debug" "FIXME Reseting modem via modemmanager"
}

# Write the radio access technology (RAT) mode to a file
function WriteConnectionRATModeFile ()
{
#	local connectionmode=`$IPC_TOOLS_DIR/umtscardtool -s AT!GETRAT? | grep !GETRAT: | awk -F": " '{print $2}'`
#	echo $connectionmode > $CONNECTION_MODE_FILE
	syslogger "error" "FIXME Get radio technology from modemmanager"
}
# Write the current network mode to a file
function WriteConnectionNetworkModeFile ()
{
#	local connectionmode
#	local cnti=`umtscardtool -s AT*CNTI=0 | grep *CNTI:`

	# *CNTI: +CME ERROR: no network service
#	local err=`expr match "$cnti" ".*\ERROR"`
#	if [ $err -eq 0 ]; then
#		connectionmode=`echo $cnti | awk -F"," '{print $2}'`
#	else
#		connectionmode=`echo $cnti | awk -F":" '{print $3}'`
#	fi
#	echo $connectionmode > $CONNECTION_MODE_FILE

	syslogger "error" "FIXME Get connection mode from modemmanager"
}

function WriteGSMCellInfoFile ()
{
	# Write Mobile Network Code
	#local gsminfo=`$IPC_TOOLS_DIR/umtscardtool -s AT!GSMINFO? | grep -E "(PLMN:|LAC:|Cell ID:).*"`
	#echo "$gsminfo" > $CONNECTION_GSMINFO_FILE
	syslogger "error" "FIXME Get GSM generic info from modemmanager"


	# Write roaming info
	#local roaming=`$IPC_TOOLS_DIR/umtscardtool -s AT^SYSINFO | grep SYSINFO | awk -F"," '{print $3}'`
	#echo -e "ROAMING:\t$roaming" >> $CONNECTION_GSMINFO_FILE
	syslogger "error" "FIXME Get GSM roaming info from modemmanager"
}

function WriteGSMConnectionInfoFiles ()
{
	# Update field strength
	WriteConnectionFieldStrengthFile

	# Write Network Mode
	WriteConnectionNetworkModeFile

	# Write GSM Information
	WriteGSMCellInfoFile
}
#-----------------------------------------------------------------------

function ReadModemStatusFile ()
{
	MODEM_STATUS=`cat $MODEM_STATUS_FILE`
	syslogger "debug" "modem status is $MODEM_STATUS"
}
function WriteModemStatusFile ()
{
	MODEM_STATUS=$1
	syslogger "debug" "modem status set to $MODEM_STATUS"
	echo $MODEM_STATUS > $MODEM_STATUS_FILE
}

function DetectModemCard ()
{
	# Sierra Wireless Modems
	if lsusb -d 1199: > /dev/null; then
		echo "/dev/usbmodem-data" > $CONNECTION_DEVICE_FILE
		echo "/dev/usbmodem-ctrl" > $COMMAND_DEVICE_FILE

		WriteModemStatusFile ${MODEM_STATES[detectedID]}
		return 0
	else
		WriteModemStatusFile ${MODEM_STATES[no_modemID]}
		return 1
	fi
}

function SetSIMPIN ()
{
	local sim_pin=`getipcoption sim.pin`

#	$UMTS_PIN $sim_pin
	syslogger "debug" "FIXME Set SIM PIN and get state via modemmanager"
	local pin_state=$?

	#  1: PIN angegeben, musste aber nicht gesetzt werden
	#  2: SIM Karte wurde nicht erkannt
	#  3: Der PIN wird benötigt, wurde aber nicht angegeben
	#  4: PUK oder SuperPIN benötigt. SIM-Karte entnehmen und mit einem Mobiltelefon entsperren.
	#  5: Die eingegebene PIN war falsch.
	#  6: Der AT-Befehl zum Setzen der PIN hat einen Fehler erzeugt.
	case $pin_state in
		0|1)
			WriteModemStatusFile ${MODEM_STATES[readyID]} ;;
		2)
			WriteModemStatusFile ${MODEM_STATES[sim_not_insertedID]} ;;
		3)
			WriteModemStatusFile ${MODEM_STATES[sim_pinID]} ;;
		*)
			WriteModemStatusFile "errorcode: $pin_state" ;;
	esac
}

# Ist die Karte eingebucht?
function CheckNIState ()
{
#	$UMTS_NI
	syslogger "debug" "FIXME Check for network booked in via modemmanager"

	local ni=$?
	if [ $ni -eq 0 ]; then
		WriteModemStatusFile ${MODEM_STATES[registeredID]}
		WriteGSMConnectionInfoFiles
	fi
	return $ni
}

function SetConnectionRAT ()
{
	ratcode=`getipcoption modem.connectionmode`
	RefreshModemDevices
	#$IPC_TOOLS_DIR/umtscardtool -d $COMMAND_DEVICE -s AT!SELRAT=$ratcode >& /dev/null
	syslogger "error" "FIXME Set the radio technology via modemmanager"
}

function InitializeModem ()
{
	DetectModemCard
	if [ $? -eq 0 ]; then
		ResetModem || syslogger "error" "Can't reset modem.'"
		SetConnectionRAT
		SetSIMPIN
		sleep 1
		WriteConnectionFieldStrengthFile
	fi
}

#-------------------------------------------------------------------------------
# WAN related functions
#-------------------------------------------------------------------------------
function get_wan_connect_current ()
{
	WAN_CURRENT=`cat $WAN_STATE_FILE`
}
function set_wan_connection_current ()
{
	local wan_curr=`cat $WAN_STATE_FILE`
	case $1 in
	reset)	wan_curr=0 ;;
	next)	wan_curr=$[$wan_curr +1]
			if [ $wan_curr -ge ${#WAN_INTERFACES[@]} ]; then
				wan_curr=0;
			fi
	esac
	echo $wan_curr > $WAN_STATE_FILE
	get_wan_connect_current
}
function shutdown_wan_connection ()
{
	if [ $START_WAN_ENABLED -eq 1 ]; then
		local wan_curr=`cat $WAN_STATE_FILE`
		if [ $# -ge 1 ]; then
			wan_curr=$1
		fi
		local wan_if=${WAN_INTERFACES[$wan_curr]}
		local wan_ct=${WAN_CHECKTARGETS[$wan_curr]}
		syslogger "debug" "Shutdown WAN connection $wan_if"
		$IPC_SCRIPTS_DIR/connection-$wan_if.sh stop
	fi
}

# Startup a connection
#   Pass connection index from config for specifix connection
function startup_wan_connection ()
{
	if [ $START_WAN_ENABLED -eq 1 ]; then
		local wan_curr=`cat $WAN_STATE_FILE`
		if [ $# -ge 1 ]; then
			wan_curr=$1
		fi
		local wan_if=${WAN_INTERFACES[$wan_curr]}
		local wan_ct=${WAN_CHECKTARGETS[$wan_curr]}
		syslogger "debug" "Starting WAN connection $wan_if"
		$IPC_SCRIPTS_DIR/connection-$wan_if.sh start
	fi
}

# This function tries to ping a target via the current interface
#   Returns 1 for unavailable error
#   Return 2 to signal restart request for interface

function check_wan_connection ()
{
	local rc=0
	if [ $START_WAN_ENABLED -eq 1 ]; then
		local wan_curr=`cat $WAN_STATE_FILE`
		if [ $# -ge 1 ]; then
			wan_curr=$1
		fi
		local wan_if=${WAN_INTERFACES[$wan_curr]}
		local wan_ct=${WAN_CHECKTARGETS[$wan_curr]}
		local wan_gw=${WAN_CHECKGATEWAYS[$wan_curr]}
		$IPC_SCRIPTS_DIR/connection-$wan_if.sh check $wan_ct $wan_gw
		rc=$?
		if [ $rc -ne 0 ]; then
			syslogger "error" "check_wan_connection: WAN interface $wan_if or ping to $wan_ct failed"
		fi
	fi
	return $rc
}
# Call connection script for current WAN interface or if arg $1 is given
#   for the indexed device .
#   connection script checks device in question and return appropriate
#   bash return value.
function check_wan_connection_status ()
{
	local rc=0
	if [ $START_WAN_ENABLED -eq 1 ]; then
		local wan_curr=`cat $WAN_STATE_FILE`
		if [ $# -ge 1 ]; then
			wan_curr=$1
		fi
		local wan_if=${WAN_INTERFACES[$wan_curr]}
		$IPC_SCRIPTS_DIR/connection-$wan_if.sh status
		if [ $? != 0 ]; then
			syslogger "error" "WAN interface $wan_if has broken status"
			rc=1;
		fi
	fi
	return $rc
}

# Call each configured backend with config options
# FIXME: Do not configure dip and ppp in parallel, won't work'
function config_wan_connections ()
{
	if [ -e /etc/ipcsystools.configmodem ] ; then
		syslogger "warn" "Found reconfigure trigger"
		for wan_if in `getipcoption general.wan.interfaces`; do
			if [ -z "$1" -o "$1" = "$wan_if" ]; then
				syslogger "warn" "Configure WAN connection $wan_if"
				$IPC_SCRIPTS_DIR/connection-$wan_if.sh config
			fi 
		done
		syslogger "warn" "Cleared reconfigure trigger"
		rm /etc/ipcsystools.configmodem
		return 0
	fi
	return 1
}

#-------------------------------------------------------------------------------
# VPN related functions
#-------------------------------------------------------------------------------

START_OVPN_ENABLED=`getipcoption ovpn.start_enabled`
START_IPSEC_ENABLED=`getipcoption ipsec.start_enabled`

function check_ipsec_status ()
{
	if [ -e $IPC_SCRIPTS_DIR/ipsec-monitor.sh ]; then
		syslogger "debug" "Checking for ipsec running"
		$IPC_SCRIPTS_DIR/ipsec-monitor.sh check > /dev/null
		if [ $? != 0 ]; then
			syslogger "warn" "IPSec not running - trigger restart"
			return 1
		fi
	fi
	return 0
}

function check_openvpn_status ()
{
	if [ $START_OVPN_ENABLED -eq 1 ]; then
	local tunnelstate="down"
	local checkmode=`getipcoption ovpn.monitorpeers`

	case "$checkmode" in
	"ifconfig"|"")
		syslogger "debug" "get ip address from client.conf"
		if [ -e /etc/openvpn/client.conf ]; then
			openvpnIP=`grep 'ifconfig' /etc/openvpn/client.conf | awk '{print $3}'`
		fi
	;;
	"watchdog")
		syslogger "debug" "get ip address from watchdog settings"
		openvpnIP=`getipcoption watchdog.ping.check_ping_ip`
	;;
	*)
		syslogger "debug"  "validate ip address"
		openvpnIP=$checkmode
	;;
	esac

	# Ping the peer address
	if (valid_ip $openvpnIP); then
		if ping -c 1 -W 5 -s 8 $openvpnIP >& /dev/null ; then
			$IPC_SCRIPTS_DIR/set_fp_leds vpn on
			tunnelstate="up"
		else
			$IPC_SCRIPTS_DIR/set_fp_leds vpn off
			tunnelstate="down"
		fi
	else
		syslogger "warn" "Wrong OpenVPN monitor peer settings ($openvpnIP)"
		tunnelstate="down"
	fi

	# Set Tunnel state
	echo $tunnelstate > $VPN_STATE_FILE
	syslogger "info" "OpenVPN monitor peer ($openvpnIP) tunnel $tunnelstate"
	fi
	[ "$tunnelstate" = "down" ] && return 1
	return 0
}

function check_vpn_connections ()
{
	local rc=0
	# Check OpenVPN status
	if [ $START_OVPN_ENABLED -eq 1 ]; then
		check_openvpn_status
		rc=$?
	fi
	# Check ipsec status
	if [ $START_IPSEC_ENABLED -eq 1 ]; then
		check_ipsec_status
		rc=$?
	fi
	return $rc;
}

function restart_vpn_connections ()
{
	# restart OpenVPN status
	if [ $START_OVPN_ENABLED -eq 1 ]  && ! check_openvpn_status ; then
		syslogger "debug" "Restarting OpenVPN connection"
		/etc/init.d/openvpn restart client &
	fi
	# Restart ipsec
	if [ $START_IPSEC_ENABLED -eq 1 ] && ! check_ipsec_status ; then
		syslogger "debug" "Restarting IPSec connection"
		/usr/sbin/ipsec restart &
	fi
}

#-----------------------------------------------------------------------
# Misc
#-----------------------------------------------------------------------

function RebootIPC()
{
	local fake=`getipcoption general.reboot.fake`
	syslogger "error" "IPC wird neu gestartet."
	if [ "$fake" -eq "1" ]; then
		syslogger "debug" "Fake mode: do not reset box."
	else
		/sbin/reboot
	fi
}

#
# Validate an IP Address
# void valid_ip (char *ipaddress)
# FIXME Handle ipv6 addresses
#
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
# Ping a target and return status RC - Called from 'connection-*.sh check'
#   <ip> <gw> <ifname>
#   <gw> := IP || default
#   A temporary host route is set to reach specific hosts behind gateways,
#    when an IP address is specified. Otherwise routing will depend on
#    existing routes.
#   The localhost address 127.0.0.1 can be used to disable this test and
#    immediately return true (rc=0)
function ping_target ()
{
	local rc=1;
	local i;
	if [ "$1" = "127.0.0.1" -a "$3" != "lo" ] ; then
		syslogger "debug" "Watchdog ping to $1 via interface $3 skipped - always true"
		return 0;
	fi

	[ "$2" != "default" ] && ip route add $1/32 via $2 dev $3;
	#bash 4.x: for $(( i=1; i<4; i++ )); do
	for i in {1..3}; do
		if ping -I $3 -c $i -W 10 -w 12 $1 >/dev/null; then
			rc=0; break;
		fi
		syslogger "warn" "Watchdog ping to $1 via interface $3 failed ($i of 3)"
	done
	[ "$2" != "default" ] && ip route del $1/32 via $2 dev $3;
	return $rc
}

#
# Check for matching mobile connection interface, and return 0 or 1
#   Support code for if-up and if-down scripts
#   Uses Env: $IFACE $PPP_IFACE
#
function check_ifupdown_on_mobile_connection
{
	local hit=1

	# PPP uses different environment variables, copy as needed
	if [ -z "$IFACE" -a -n "$PPP_IFACE" ] ; then
		IFACE=$PPP_IFACE
	fi

	# Check if current interface matches IPC mobile connection
	wan_ifs="`getipcoption general.wan.interfaces`"
	syslogger "debug" "Is Interface $IFACE a mobile connection? wan_ifs=$wan_ifs"
	for wan_if in $wan_ifs; do
		wan_dev=`getipcoption connection.$wan_if.dev`
		if [ "$IFACE" = "$wan_dev" ]; then
			hit=0; break
		fi
	done

	syslogger "warn" "Is Interface $IFACE a mobile connection? rc=$hit wan_if=$wan_if wan_dev=$wan_dev"
	return $hit
}
# Outputs the name of the statusled to set
function get_on_mobile_connection_led
{
	local led=""

	# PPP uses different environment variables, copy as needed
	if [ -z "$IFACE" -a -n "$PPP_IFACE" ] ; then
		IFACE=$PPP_IFACE
	fi

	if [ '$PPP_TTYNAME' = 'usbmodem-data' ] ; then
		led="3g"
	fi

	# Check if current interface matches IPC mobile connection
	wan_ifs="`getipcoption general.wan.interfaces`"
	for wan_if in $wan_ifs; do
		if [ "$IFACE" = "`getipcoption connection.$wan_if.dev`" ]; then
			led=`getipcoption connection.$wan_if.statusled`
			if [ -z "$led" ] ; then led="3g"; fi
			break
		fi
	done
	syslogger "warn" "Get statusled for interface $IFACE : '$led'"
	echo "$led"
}
