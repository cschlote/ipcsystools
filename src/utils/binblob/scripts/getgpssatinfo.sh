#!/bin/bash
#set -x
let value=0
if [ -e /var/run/gps_satinfo ]; then
	str=`cat /var/run/gps_satinfo | sed -n -e 's~Satellites in view:[ ]*\([0-9]*\)~\1~p'`
	if [ -n "$str" ]; then
		let value=$str
	fi
fi
echo $value

