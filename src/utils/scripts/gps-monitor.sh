#!/bin/bash

#
# GPS Function Library
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

DESC="gps-monitor[$$]"

GPS_STATUS_FILE=$MCB_STATUSFILE_DIR/gps_status
GPS_LOCATION_FILE=$MCB_STATUSFILE_DIR/gps_location
GPS_SATINFO_FILE=$MCB_STATUSFILE_DIR/gps_satinfo

MCB_MONITOR_PID_FILE=/var/lock/gps-monitor.pid

# ----------------------------------------------------------------------
# Errorcodes
# ----------------------------------------------------------------------

# AT command error codes (!GPSEND, !GPSSTATUS, !GPSTRACK)

GPS_Errorcode=(
"Phone is offline"
"No service"
"No connection with PDE (Position Determining Entity)"
"No data available"
"Session Manager is busy"
"Reserved"
"Phone is GPS-locked"
"Connection failure with PDE"
"Session ended because of error condition"
"User ended the session"
"End key pressed from UI"
"Network session was ended"
"Timeout (for GPS search)"
"Conflicting request for session and level of privacy"
"Could not connect to the network"
"Error in fix"
"Reject from PDE"
"Reserved"
"Ending session due to E911 call"
"Server error"
"Reserved"
"Reserved"
"Unknown system error"
"Unsupported service"
"Subscription violation"
"Desired fix method failed"
"Reserved"
"No fix reported because no Tx confirmation was received"
"Network indicated normal end of session"
"No error specified by the network"
"No resources left on the network"
"Position server not available"
"Network reported an unsupported version of protocol"
)

# AT command error codes (!GPSFIX)

GPS_ErrorCodeFix=(
"No error"
"Invalid client ID"
"(MC8775V / 80 / 81 / 90 / 90V) Bad service parameter (MC8785V) Reserved"
"Bad session type parameter"
"(MC8775V / 80 / 81 / 90 / 90V) Incorrect privacy parameter (MC8785V) Reserved"
"(MC8775V / 80 / 81 / 90 / 90V) Incorrect download parameter (MC8785V) Reserved"
"(MC8775V / 80 / 81 / 90 / 90V) Incorrect network access parameter (MC8785V) Reserved"
"Incorrect operation parameter"
"Incorrect number of fixes parameter"
"Incorrect server information parameter"
"Error in timeout parameter"
"Error in QOS accuracy threshold parameter"
"No active session to terminate"
"(MC8775V / 80 / 81 / 90 / 90V) Session is active (MC8785V) Reserved"
"Session is busy"
"Phone is offline"
"Phone is CDMA locked"
"GPS is locked"
"Command is invalid in current state"
"Connection failure with PDE"
"PDSM command buffer unavailable to queue command"
"Search communication problem"
"(MC8775V / 80 / 81 / 90 / 90V) Temporary problem reporting position determination results (MC8785V) Reserved"
"(MC8775V / 80 / 81 / 90 / 90V) Error mode not supported (MC8785V) Reserved"
"Periodic NI in progress"
"(MC8775V / 80 / 81 / 90 / 90V) Unknown error (MC8785V) Client authentication failure"
"(MC8785V) Unknown error"
)

# ----------------------------------------------------------------------
# Modem Handling
# ----------------------------------------------------------------------

# at!custom?
# !CUSTOM:
#             PUKPRMPT            0x01
#             MEPCODE             0x01
#             ISVOICEN            0x01
#             PRLREGION           0x01
#             PCSCDISABLE         0x03
#             GPSENABLE           0x01  (or not displayed when not configured)
# OK
function gps_checkmodem_feature ()
{
    syslogger "debug" "Check modem caps"
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!custom? | grep -q GPSENABLE; then
	syslogger "error" "Modem has no GPS feature support"
	return 1
    fi
    return 0
}

# at!gpsautostart=?
# !GPSAUTOSTART: <enable>[,<fixtype>,<maxtime>,<maxdist>,<fixrate>]
# <enable>:  0-Disabled, 1-Enabled
# <fixtype>: 1-Standalone, 2-MS-Based, 3-MS-Assisted
# <maxtime>: 1-255 seconds
# <maxdist>: 1-4294967280 meters
# <fixrate>: 1-65535 seconds
function gps_gpsautostart ()
{
    local ena=1; [ -n "$1" ] && mode=$1
    local mode=1; [ -n "$2" ] && mode=$2
    local to=15; [ -n "$3" ] && to=$3
    local dist=30; [ -n "$4" ] && dist=$4
    local fr=10; [ -n "$4" ] && dist=$4
    syslogger "debug" "Send at!gpsautostartx=$ena,$mode,$to,$dist,$fr"
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpsautostartx=$ena,$mode,$to,$dist,$fr | grep -q OK; then
	syslogger "error" "Modem failed on !gpsautostartx command"
	return 1
    fi
    return 0
}


# AT!GPSFIX=?
# AT!GPSFIX: <fixtype>,<maxtime>,<maxdist>
# <fixtype>: 1-Standalone, 2-MS-Based, 3-MS-Assisted
# <maxtime>: 0-255 seconds
# <maxdist>: 0-4294967280 meters
# AT!GPSFIX=1,15,10
# 
# OK
function gps_gpsfix ()
{
    local mode=1; [ -n "$1" ] && mode=$1
    local to=15; [ -n "$2" ] && to=$2
    local dist=30; [ -n "$3" ] && dist=$3
    syslogger "debug" "Send at!gpsfix=$mode,$to,$dist command "
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpsfix=$mode,$to,$dist | grep -q OK; then
	syslogger "error" "Modem failed on !gpsfix command"
	return 1
    fi
    return 0
}

# AT!GPSSTATUS?
# Current time: <y> <m> <d> <dow> <time>
# 
# <y> <m> <d> <dow> <time> Last Fix Status    = <status>
# <y> <m> <d> <dow> <time> Fix Session Status = <status>
# 
# TTFF (sec) = <n>
#
# OK
# <status>:=NONE ACTIVE SUCCESS FAIL
function gps_gpsstatus ()
{
    syslogger "debug" "Send at!gpsstatus? command"
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpsstatus?; then
	syslogger "error" "!gpsstatus reports failed"
	return 1
    fi
    return 0
}

# AT!GPSLOC?
# Lat: 50 Deg 34 Min 58.69 Sec N  (0x008FE165)
# Lon: 8 Deg 32 Min 31.62 Sec E  (0x00184C2E)
# Time: 2010 07 07 2 10:58:44 (GPS)
# LocUncAngle: 1  LocUncA: 21  LocUncP: 19  HEPE: f
# 3D Fix
# Altitude: 217  LocUncVe: 20
#
# OK
function gps_gpsloc ()
{
    syslogger "debug" "Send at!gpsloc? command "
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpsloc?; then
	syslogger "error" "!gpsloc reports failed"
	return 1
    fi
    return 0
}

# !GPSTRACK: <fixtype>,<maxtime>,<maxdist>,<fixcount>,<fixrate>
# <fixtype>: 1-Standalone, 2-MS-Based, 3-MS-Assisted
# <maxtime>: 0-255 seconds
# <maxdist>: 0-4294967280 meters
# <fixcount>: 1-1000,1000=continuous
# <fixrate>: 1-1,799,999 seconds

# AT!GPSTRACK=1,255,30,1000,1
function gps_gpstrack ()
{
    local mode=1; [ -n "$1" ] && mode=$1
    local to=255; [ -n "$2" ] && to=$2
    local dist=30; [ -n "$3" ] && dist=$3
    local fc=1000; [ -n "$4" ] && fc=$4
    local fr=1; [ -n "$5" ] && fr=$5
    syslogger "debug" "Send at!gpstrack=$mode,$to,$dist,$fc,$fr command "
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpstrack=$mode,$to,$dist,$fc,$fr | tr -d '\r'; then
	syslogger "error" "!gpstrack reports failed"
	return 1
    fi
    return 0
}

function gps_gpssatinfo ()
{
    syslogger "debug" "Send at!gpssatinfo? command "
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpssatinfo? | tr -d '\r'; then
	syslogger "error" "!gpssatinfo? failed"
	return 1
    fi
    return 0
}

function gps_gpsend ()
{
    syslogger "debug" "Send at!gpsend=0 command "
    if ! $MCB_TOOLS_DIR/umtscardtool -s at!gpsend=0 | tr -d '\r'; then
	syslogger "error" "!gpsend=0 failed"
	return 1
    fi
    return 0
}

#-----------------------------------------------------------------------
# GPS functions
#-----------------------------------------------------------------------

GPS_TIME=
GPS_LASTSTATUS=
GPS_CURRSTATUS=
GPS_LASTERROR=
GPS_CURRERROR=
GPS_TTFF=

function save_gpsstatus ()
{
    gps_gpsstatus | tr -d '\r' | grep -v "OK" >$GPS_STATUS_FILE
}
function save_gpslocation ()
{
    gps_gpsloc | tr -d '\r' | grep -v "OK" >$GPS_LOCATION_FILE
}
function save_gpssatinfo ()
{
    gps_gpssatinfo | tr -d '\r' | grep -v "OK" >$GPS_SATINFO_FILE
}
function get_errorstring ()
{
    if [ -n "$1" ]; then
	echo ${GPS_Errorcode[$1]}
    fi
}
function query_gpsstatus ()
{
    save_gpsstatus
    # cat $GPS_STATUS_FILE
    GPS_TIME=`cat $GPS_STATUS_FILE | sed -n "s/^Current time: \(.*\)$/\1/ p" | tr -d ' :'`
    GPS_TTFF=`cat $GPS_STATUS_FILE | sed -n "s/^TTFF (sec) = \(.*\)$/\1/ p" | tr -d ' :'`
    local t1=`cat $GPS_STATUS_FILE | sed -n "s/^.*Last Fix Status    = \(.*\)$/\1/ p"`
    local t2=`cat $GPS_STATUS_FILE | sed -n "s/^.*Fix Session Status = \(.*\)$/\1/ p"`
    GPS_LASTSTATUS=$t1
    GPS_CURRSTATUS=$t2
    GPS_LASTERROR=`echo $t1 | sed -n "s/.*FAILCODE = \(.*\)$/\1/ p"`
    GPS_CURRERROR=`echo $t2 | sed -n "s/.*FAILCODE = \(.*\)$/\1/ p"`
    echo "Current GPS time: $GPS_TIME"
    echo "Last status: $GPS_LASTSTATUS `get_errorstring $GPS_LASTERROR`"
    echo "Curr status: $GPS_CURRSTATUS `get_errorstring $GPS_CURRERROR`"
    echo "Current GPS TTFF: $GPS_TTFF"
}
function query_gpsloc ()
{
    save_gpslocation
    cat $GPS_LOCATION_FILE
}
function query_gpssatinfo ()
{
    save_gpssatinfo
    cat $GPS_SATINFO_FILE
}


function issue_gpsfix ()
{
    gps_gpsfix "$1" "$2" "$3"
}
function issue_gpstrack ()
{
    gps_gpstrack "$1" "$2" "$3" "$4" 
}
function wait_gpsfix ()
{
    local rc=0
    while true; do
	query_gpsstatus
	case "$GPS_CURRSTATUS" in
	NONE)
	    syslogger "error" "No gpsfix started"
	    rc=1; break ;;
	ACTIVE)
	    syslogger "debug" "gpsfix started and active"
	    ;;
	SUCCESS)
	    syslogger "debug" "gpsfix succesfully finished"
	    rc=0; break;;
	FAIL*)
	    syslogger "debug" "gpsfix failed"
	    rc=1; break;;
	*)
	    syslogger "debug" "unknown currstatus $GPS_CURRSTATUS"
	    rc=1; break
	esac

#	case "$GPS_LASTSTATUS" in
#	NONE*)		;;
#	SUCCESS*)	;;
#	FAIL*)		;;
#	*)
#	    syslogger "debug" "unknown laststatus"
#	    rc=1; break
#	esac

	sleep 10
    done

    if [ $rc = 1 ]; then
	syslogger "error" "!gpsstatus reports failed or none fixup"
	#local ec=`echo ${status/\r/} | awk '{print $13}'`
	#echo "Error $ec := ${GPS_Errorcode[$ec]}"
	return 1
    fi
    return 0
}

#-----------------------------------------------------------------------
# MCB GPS Monitor
#-----------------------------------------------------------------------

function start_gpsd ()
{
    issue_gpstrack 1 15 100 1
    query_gpsstatus
  
    if [ "$GPS_CURRSTATUS" = "ACTIVE" ]; then
	if pidof gpsd; then
	    gpsd -P /var/run/gpsd.pid /dev/ttyUSB2 &
	fi
    else
	syslogger "debug" "Unable to start tracking session"
    fi
}

function stop_gpsd ()
{
    if pidof gpsd && [ -e /var/run/gpsd.pid ]; then
	kill -TERM 'cat /var/run/gpsd.pid'
	rm /var/run/gpsd.pid
    fi
}

function gps_start ()
{
    # --- Get an intitial GPS fix ----------------------------
    query_gpsstatus
    if [ "$GPS_CURRSTATUS" = "ACTIVE" ]; then
	syslogger "debug" "Stopping active GPS Fix session"
	gps_gpsend
    fi

    local retries=3
    while [ $retries -gt 0 ]; do
	syslogger "debug" "Trying to obtain initial fix (retries=$retries)"
	issue_gpsfix 1 255 100
	wait_gpsfix
	retries=$[$retries-1]
	query_gpsloc
	query_gpssatinfo
    done

    if [ $retries -eq 0 ]; then
	syslogger "error" "Unable to get GPS fix, yet"
    fi

    # --- Start NMEA output on USB port
    start_gpsd
}

function gps_stop ()
{
    stop_gpsd
}

function gps_monitor ()
{
    query_gpsstatus
    query_gpsloc
    query_gpssatinfo
}

function print_usage {
    echo "Usage: -h <start|stop|monitor>"
}

echo -e "\nkonzeptpark GPS monitor\n"

while getopts "hfd:s:r:" optionName; do
	case "$optionName" in
	h) print_usage; exit 1;;
#	f) force=1;;			# FIXME: Set values for to,dist, ...
#	d) device="$OPTARG";;
#	s) serverip="$OPTARG";;
#	r) versionpath="$OPTARG";;
	[?]) print_usage; exit 1;;
	esac
done

if ! [ $# -ge 1 ]; then
    print_usage; exit 1
fi

obtainlock $MCB_MONITOR_PID_FILE
syslogger "debug" "Started monitor (`date`)"

# Update files and links for GSM modem connection
WriteGSMConnectionInfoFiles

if gps_checkmodem_feature; then

    case "$1" in
	"start")	gps_start ;;
	"stop")	gps_stop ;;
	"monitor")	gps_monitor ;;
    esac

else
    echo "Modem doesn't support GPS"
fi

syslogger "debug" "Finished monitor (`date`)"
releaselock
