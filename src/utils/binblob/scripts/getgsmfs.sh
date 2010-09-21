#!/bin/bash
#set -x
value=`cat /var/run/connection_fs`
if [ $value -lt 0 -o $value -gt 31 ]; then
	let value=99
fi
echo $value

