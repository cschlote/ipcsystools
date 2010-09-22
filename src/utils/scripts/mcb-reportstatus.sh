#!/bin/bash
#
# DESCRIPTION: Report status as BASE64 string
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

DESC="mcb-reportstatus[$$]"

MCB_REPORTER_PID_FILE=/var/lock/mcb-reportstatus.pid
MCB_REPORTER_STATUS=/var/run/mcb-reportstatus-status

REPORT_ENABLED=`getmcboption reportstatus.enable`
REPORT_IP_TARGET=`getmcboption reportstatus.ip.target`
REPORT_IP_PORT=`getmcboption reportstatus.ip.port`
REPORT_LEVEL=`getmcboption reportstatus.level`
REPORT_INTERVAL=`getmcboption reportstatus.interval`


obtainlock $MCB_REPORTER_PID_FILE
syslogger "debug" "Started mcb report (`date`)"

#-- Start of script ------------------------------------------------------
if [ "$REPORT_ENABLED" -eq "1" ]; then
	cd $MCB_SCRIPTS_DIR

	if [ ! -e $MCB_REPORTER_STATUS ]; then
		syslogger "debug" "First run, send full version string"
		echo -n "0" > $MCB_REPORTER_STATUS
		binblob -s asciimsg.script base64 | nc -c $REPORT_IP_TARGET $REPORT_IP_PORT
		REPORT_LEVEL=5
	fi

	let tdiff=`date +%s`-`cat $MCB_REPORTER_STATUS`
	let tdiff=$tdiff-$REPORT_INTERVAL

	if [ $tdiff -ge 0 ]; then
		syslogger "debug" "Reporting mcb status"
		date +%s > $MCB_REPORTER_STATUS
		for i in mcb2msg$REPORT_LEVEL.script; do
			binblob -s $i base64 | nc -c $REPORT_IP_TARGET $REPORT_IP_PORT
		done;
	else
		syslogger "debug" "Reporting next mcb status in $tdiff seconds"
	fi
fi
#-- End of script ------------------------------------------------------
syslogger "debug" "Finished mcb report (`date`)"
releaselock
