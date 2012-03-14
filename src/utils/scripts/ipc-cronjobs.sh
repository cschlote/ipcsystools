#!/bin/bash
#
# DESCRIPTION: Start the different cron jobs.
#
/usr/bin/ipc-monitor.sh

/usr/bin/ipcsystools/gps-monitor.sh monitor > /dev/null

/usr/bin/ipc-reportstatus.sh
