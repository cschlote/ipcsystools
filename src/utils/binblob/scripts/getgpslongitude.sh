#!/bin/bash
#set -x
let temp=0
if [ -e /var/run/gps_location ]; then
	value=(`cat /var/run/gps_location | sed -n -e 's~Lon: \(.*\) Deg \(.*\) Min \(.*\)\.\(.*\) Sec \(.*\)  .*~\1 \2 \3 \4 \5~p'`)
	if [ ${#value[@]} -eq 5 ]; then
		let temp1=${value[0]}*60*10000
		let temp2=10000*${value[1]}
		let temp3=${value[2]}*10000/60
		let temp4=${value[3]}*10000/6000
		let temp=$temp1+$temp2+$temp3+$temp4
		if [ "${value[4]}" != "E" ]; then
			let temp=0-$temp
		fi
	fi
fi	
echo $temp

