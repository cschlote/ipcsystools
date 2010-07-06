#!/bin/bash
#
# DESCRIPTION: Script for control the Leds on the MCB-2
#

# LED names 
ALL_LEDS=( ready error vpn gsm 3g gpsfix option1 option2 service gsm-fs gps-fs bar0 bar1 bar2 bar3 bar4 bar5 bar6 bar7 )
BAR_LEDS=( bar0 bar1 bar2 bar3 bar4 bar5 bar6 bar7 )

# Defines for paths and values
LED_CMD_ON_OFF="/sys/class/leds/leds:%s/brightness\n"
LED_CMD_TIMER="/sys/class/leds/leds:%s/trigger\n"
LED_CMD_TIMER_DON="/sys/class/leds/leds:%s/delay_on\n"
LED_CMD_TIMER_DOFF="/sys/class/leds/leds:%s/delay_off\n"
LED_ON=255
LED_OFF=0

# Control single LEDs
led_on () {
	local ledcmd=`printf $LED_CMD_ON_OFF $1`
	echo $LED_ON > $ledcmd
}
led_off () {
	local ledcmd=`printf $LED_CMD_ON_OFF $1`
	echo $LED_OFF > $ledcmd
}

# Vanilla functions
led_all_off () {
	for i in "${ALL_LEDS[@]}"; do
		led_off $i
	done
}
led_bar_off () {
	for i in "${BAR_LEDS[@]}"; do
		led_off $i
	done
}

# Setup blinking LED
#
# <name> <on_time> <off_time>
#
led_timer()
{    
  local ledcmd=`printf $LED_CMD_TIMER $1`
  local delay_on=`printf $LED_CMD_TIMER_DON $1`
  local delay_off=`printf $LED_CMD_TIMER_DOFF $1`
  
  echo timer > $ledcmd
  echo $2 > $delay_on
  echo $3 > $delay_off
}

# Setup bargraph for fieldstrength
#
# <fs>
#
led_gsmfs()
{
	local fsnorm
	local delay_on=`printf $LED_CMD_TIMER_DON gsm-fs`

	# Convert fieldsrength 0..31 to bargraph value
	case "$1" in
		0|1) 			fsnorm=0	;;
		2|3)			fsnorm=1	;;
		4|5)			fsnorm=2	;;
		6|7|8|9)		fsnorm=3	;;    
		10|11|12|13)	fsnorm=4	;;
		14|15|16|17)	fsnorm=5	;;  
		18|19|20|21)	fsnorm=6	;;
		22|23|24|25)	fsnorm=7	;;
		26|27|28|29|30|31)
						fsnorm=8	;;
		*)				fsnorm=0	;;
	esac  

	# GSM-FS Led on
	if [ $1 -gt 32 ]; then
		led_timer gsm-fs 1000 2000
	else 		
		if [ -e $delay_on ]; then
			led_off gsm-fs
		else
			led_on gsm-fs
		fi
	fi

	# Enable bargraph
	local i=0
	for led in "${BAR_LEDS[@]}"; do
		if [ $i -lt $fsnorm ]; then
			led_on $led  
		else
			led_off $led
		fi 
		let i+=1
	done
}

#
# GPS Sat's in view
#
led_gpsfs () {
	led_bar_off
	logger -s  "led_gpsfs() not implemented"
}

show_usage () {
	cat << EOT
Usage: leds.sh
	<"3g"|"vpn"|"gpsfix"|"option1"|"option2"|"service">  <on|off>
	<"gsmfs"|"gpsfs"> <fieldstrengh>
	<"all-off"> 
EOT
}
#
# Main routine
#
case "$1" in  
	"3g"|"vpn"|"gpsfix"|"option1"|"option2"|"service")
					case "$2" in
					  "on") 
						# LED erst ausschalten wegen Timermode
						led_off $1
						led_on $1
					  ;;
					  "off") 
						led_off $1
					  ;;      
					  "timer") 
						#TODO: Zeit als Parameter übergeben
						led_timer $1 500 500
					  ;;
					esac			;;
	"gsmfs")		led_gsmfs $2	;;
	"gpsfs")	    led_gpsfs $2	;;
	"all-off")		led_all_off		;;
	"") 			show_usage 		;;	
	*) 
		logger -s "$0: unknown parameter ´$*´" >&2
		exit 2    
	;;
esac

exit 0
