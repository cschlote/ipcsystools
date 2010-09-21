#!/bin/bash
#set -x
value=`cat /etc/ptxdist_version | cut -d' ' -f2  | sed -n -e 's~kp-mcb2-\(.*\)\..*~\1~p'`
echo $value

