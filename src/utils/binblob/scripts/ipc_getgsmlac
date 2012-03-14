#!/bin/bash
#set -x
value=0
if [ -e /var/run/connection_gsminfo ]; then
	value="`cat /var/run/connection_gsminfo | sed -n -e 's~LAC:.\(.*\)~\1~p'`"
fi
echo $value

