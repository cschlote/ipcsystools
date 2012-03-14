#!/bin/bash
#set -x
value="`ip addr show dev eth0 | sed -n  -e 's~.*inet \([0-9]*\)\.\([0-9]*\).\([0-9]*\).\([0-9]*\)/.*~\1 \2 \3 \4~p'`"
let temp=0
for i in $value; do
	let temp=$temp*1000
	let temp=$temp+$i
done
echo $temp

