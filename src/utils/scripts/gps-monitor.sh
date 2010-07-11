#!/bin/bash -u -x
#
# GPS Function Library
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

DESC="gps-monitor[$$]"

MCB_MONITOR_PID_FILE=/var/lock/mcb-monitor.pid

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
$MCB_TOOLS_DIR/umtscardtool -s ATZ

# at!entercnd="A710"
# at!custom="GPSENABLE",1
# at!greset

# ----------------------------------------------------------------------
# -- ContinusStart Tracking to ttyUSB2
# ----------------------------------------------------------------------

# !GPSTRACK: <fixtype>,<maxtime>,<maxdist>,<fixcount>,<fixrate>
# <fixtype>: 1-Standalone, 2-MS-Based, 3-MS-Assisted
# <maxtime>: 0-255 seconds
# <maxdist>: 0-4294967280 meters
# <fixcount>: 1-1000,1000=continuous
# <fixrate>: 1-1,799,999 seconds

# AT!GPSTRACK=1,255,30,1000,1

# ----------------------------------------------------------------------
# -- Standalone fix
# ----------------------------------------------------------------------

# AT!GPSFIX=?
# AT!GPSFIX: <fixtype>,<maxtime>,<maxdist>
# <fixtype>: 1-Standalone, 2-MS-Based, 3-MS-Assisted
# <maxtime>: 0-255 seconds
# <maxdist>: 0-4294967280 meters

# AT!GPSFIX=1,15,10

# ----------------------------------------------------------------------
# -- Monitor current fix status
# ----------------------------------------------------------------------

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

# ----------------------------------------------------------------------
# -- Query current position
# ----------------------------------------------------------------------

# AT!GPSLOC?
# Lat: 50 Deg 34 Min 58.69 Sec N  (0x008FE165)
# Lon: 8 Deg 32 Min 31.62 Sec E  (0x00184C2E)
# Time: 2010 07 07 2 10:58:44 (GPS)
# LocUncAngle: 1  LocUncA: 21  LocUncP: 19  HEPE: f
# 3D Fix
# Altitude: 217  LocUncVe: 20
#
# OK



#-----------------------------------------------------------------------
# MCB GPS Monitor
#-----------------------------------------------------------------------

obtainlock $MCB_MONITOR_PID_FILE
syslogger "debug" "Started monitor (`date`)"

# Update files and links for GSM modem connection
WriteGSMConnectionInfoFiles

#-- Start of script ----------------------------------------------------



#-- End of script ------------------------------------------------------
syslogger "debug" "Finished monitor (`date`)"
releaselock
exit 0
