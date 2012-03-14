#!/bin/bash
#set -x
rc=0
if [ -e /var/run/connection_mode ]; then
	value=`cat /var/run/connection_mode`
	case $value in
	UMTS*) rc=2;;
	GPRS*) rc=1;;
	esac
fi;
echo $rc

