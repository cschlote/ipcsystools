#!/bin/bash
#set -x
let value=`cat /sys/bus/w1/devices/10-*/w1_slave | sed -n -e 's~.*t=\(.*\)~\1~p'`
let temp=0
let temp=$value+500
let temp=$temp/1000 
let temp=$temp+40
echo $temp

