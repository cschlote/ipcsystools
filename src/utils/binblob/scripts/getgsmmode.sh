#!/bin/bash
#set -x
value=`cat /var/run/connection_mode`
rc=0
case $value in
UMTS*) rc=2;;
GPRS*) rc=1;;
esac
echo $rc

