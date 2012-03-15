#!/bin/bash
#
# DESCRIPTION: Start the different cron jobs.
#
/usr/bin/ipc-monitor

/usr/bin/gps-monitor monitor

/usr/bin/ipc-reportstatus
