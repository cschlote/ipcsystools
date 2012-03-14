#!/bin/bash
#
# DESCRIPTION: Start the different cron jobs.
#
/usr/bin/ipc-monitor

/usr/bin/ipcsystools/gps-monitor monitor > /dev/null

/usr/bin/ipc-reportstatus
