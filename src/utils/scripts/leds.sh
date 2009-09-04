#!/bin/bash
#**********************************************************************************
#
#        FILE: leds.sh
#
#       USAGE: leds.sh 3g on ...
#
# DESCRIPTION: Script for control the Leds on the MCB-2
#
#      AUTHOR: Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY: konzeptpark GmbH, 35633 Lahnau
#
#**********************************************************************************

ALL_LEDS=( ready error vpn gsm 3g gpsfix option1 option2 service gsm-fs gps-fs bar0 bar1 bar2 bar3 bar4 bar5 bar6 bar7 )
BAR_LEDS=( bar0 bar1 bar2 bar3 bar4 bar5 bar6 bar7 )

LED_CMD_ON_OFF="/sys/class/leds/leds:%s/brightness\n"
LED_CMD_TIMER="/sys/class/leds/leds:%s/trigger\n"
LED_CMD_TIMER_DON="/sys/class/leds/leds:%s/delay_on\n"
LED_CMD_TIMER_DOFF="/sys/class/leds/leds:%s/delay_off\n"
LED_ON=255
LED_OFF=0

# Alle LED's ausschalten
led_all_off()
{
  for i in "${ALL_LEDS[@]}"
  do
    led_off $i
  done
}

# Alle LED's der Balkenanzeige ausschalten
led_bar_off()
{
  for i in "${BAR_LEDS[@]}"
  do
    led_off $i
  done
}

# LED einschalten
led_on()
{
  local ledcmd=`printf $LED_CMD_ON_OFF $1`
  echo $LED_ON > $ledcmd
}
# LED ausschalten
led_off()
{
  local ledcmd=`printf $LED_CMD_ON_OFF $1`
  echo $LED_OFF > $ledcmd
}
# LED Timer gesteuert
led_timer()
{    
  local ledcmd=`printf $LED_CMD_TIMER $1`
  local delay_on=`printf $LED_CMD_TIMER_DON $1`
  local delay_off=`printf $LED_CMD_TIMER_DOFF $1`
  
  echo timer > $ledcmd
  echo $2 > $delay_on
  echo $3 > $delay_off
}

# Balkenanzeige für GSM Signalstaerke
led_gsmfs()
{
  local fsnorm
	local delay_on=`printf $LED_CMD_TIMER_DON gsm-fs`

  # Normierung der Feldstärke
  case "$1" in
    0|1) 
      fsnorm=0;;
		2|3)
			fsnorm=1;;
    4|5)
      fsnorm=2;;
    6|7|8|9)
      fsnorm=3;;    
    10|11|12|13)
      fsnorm=4;;
    14|15|16|17)
      fsnorm=5;;  
    18|19|20|21)
      fsnorm=6;;
    22|23|24|25)
      fsnorm=7;;
    26|27|28|29|30|31)
      fsnorm=8;;
    *)
      fsnorm=0;;
  esac  

  # GSM-FS Led on
  if [ $1 -gt 32 ]; then
    led_timer gsm-fs 1000 2000
  else 		
		test -e $delay_on && led_off gsm-fs
    led_on gsm-fs
  fi

  # Balkenanzeige LED's on
  local i=0
  for led in "${BAR_LEDS[@]}"
  do    
    if [ $i -lt $fsnorm ]; then
      led_on $led  
    else
      led_off $led
    fi 
  
    let i+=1
  done
}

# GPS Sat's in view
led_gpsfs()
{
  echo "led_gpsfs()"
}


# -- MAIN ---

case "$1" in  
  # Status LED's
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
    # Default!!
    esac
  
  ;;

  # GSM Field Strength
  "gsmfs")
    led_gsmfs $2 # Parameter prüfen!!
  ;;

  # GPS Satellites in view
  "gpsfs")
    led_bar_off
    led_gpsfs
  ;;

	# Alle LED's
	"all-off")
		led_all_off		
	;;

  # Default
  *) 
    echo "$0: unknown parameters \`$*'" >&2
	  exit 2    
  ;;
 
esac

exit 0


