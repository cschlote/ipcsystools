#!/bin/bash
#set -x
value=(`cat /var/run/gps_status | sed -n -e 's/^Current time: \(.*\)$/\1/ p' | tr ':' ' '`)
#echo ${value[@]}
gpstime=${value[0]}${value[1]}${value[2]}${value[4]}${value[5]}.${value[6]}
date --date=$gpstime +%s

