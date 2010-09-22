#!/bin/bash
#
# DESCRIPTION: Monitor the WAN and VPN Connections
#

/usr/bin/mcb-monitor.sh

/usr/bin/gps-monitor.sh monitor

/usr/bin/mcb-reportstatus.sh
