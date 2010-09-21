#!/bin/bash
#set -x
value=`cat /var/run/connection_gsminfo | sed -n -e 's~PLMN:.\(.*\)~\1~p'`
echo $value

