#!/bin/bash
#
# DESCRIPTION: Report status as BASE64 string
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/ipcsystools/ipclib.inc

DESC="ipc-reportstatus[$$]"

IPC_REPORTER_PID_FILE=/var/lock/ipc-reportstatus.pid
IPC_REPORTER_STATUS=/var/run/ipc-reportstatus-status

REPORT_ENABLED=`getipcoption reportstatus.enable`
REPORT_IP_TARGET=`getipcoption reportstatus.ip.target`
REPORT_IP_PORT=`getipcoption reportstatus.ip.port`
REPORT_LEVEL=`getipcoption reportstatus.level`
REPORT_INTERVAL=`getipcoption reportstatus.interval`


obtainlock $IPC_REPORTER_PID_FILE
syslogger "debug" "Started ipc report (`date`)"

#-- Start of script ------------------------------------------------------
if [ "$REPORT_ENABLED" -eq "1" ]; then
	cd $IPC_SCRIPTS_DIR

	if [ ! -e $IPC_REPORTER_STATUS ]; then
		syslogger "debug" "First run, send full version string"
		echo -n "0" > $IPC_REPORTER_STATUS
		binblob -s asciimsg.script base64 | nc -c $REPORT_IP_TARGET $REPORT_IP_PORT
		REPORT_LEVEL=5
	fi

	let tdiff=`date +%s`-`cat $IPC_REPORTER_STATUS`
	let tdiff=$tdiff-$REPORT_INTERVAL

	if [ $tdiff -ge 0 ]; then
		syslogger "debug" "Reporting ipc status"
		date +%s > $IPC_REPORTER_STATUS
		for i in ipc2msg$REPORT_LEVEL.script; do
			binblob -s $i base64 | nc -c $REPORT_IP_TARGET $REPORT_IP_PORT
		done;
	else
		syslogger "debug" "Reporting next ipc status in $tdiff seconds"
	fi
fi
#-- End of script ------------------------------------------------------
syslogger "debug" "Finished ipc report (`date`)"
releaselock
