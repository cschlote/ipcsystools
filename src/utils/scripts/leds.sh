#!/bin/bash
#**********************************************************************************
#
#        FILE: leds.sh
#
#       USAGE: leds.sh 3g on ...
#
# DESCRIPTION: Script for control the Leds (FAKE)
#
#      AUTHOR: Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY: konzeptpark GmbH, 35633 Lahnau
#
#**********************************************************************************

ALL_LEDS=( ready hd error vpn gsm 3g gpsfix option1 option2 )

LED_CMD_ON_OFF="/tmp/leds:%s_brightness\n"
LED_CMD_TIMER="/tmp/leds:%s_trigger\n"
LED_CMD_TIMER_DON="/tmp/leds:%s_delay_on\n"
LED_CMD_TIMER_DOFF="/tmp/leds:%s_delay_off\n"
LED_ON=255
LED_OFF=0

LED_TOOL="/usr/bin/ipc-set-led"

# Check for Led tool
test -x $LED_TOOL || exit 0

# Alle LED's ausschalten
led_all_off()
{
  $LED_TOOL all-off
}
# LED einschalten
led_on()
{  
  $LED_TOOL $1 on
}
# LED ausschalten
led_off()
{
  $LED_TOOL $1 off
}
# LED Timer gesteuert
led_timer()
{    
  # currently not implemented  
  $LED_TOOL $1 trigger
}

# -- MAIN ---

case "$1" in  
  # Status LED's
  "ready"|"error"|"vpn"|"3g"|"gpsfix"|"option1"|"option2")

    case "$2" in
      "on") 
        led_on $1
      ;;
      "off") 
        led_off $1
      ;;      
      "timer")
        # currently not implemented
      ;;
    # Default!
    esac  
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
