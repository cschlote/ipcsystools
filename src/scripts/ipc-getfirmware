#!/bin/bash
#set -x
value=unknown
if [ -e /etc/ptxdist_version ]; then
	value=`cat /etc/ptxdist_version | cut -d' ' -f2  | sed -n -e 's~kp-mcb2-\(.*\)\..*~\1~p'`
fi
echo $value

