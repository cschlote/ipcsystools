#!/bin/sh
#**********************************************************************************
#
#        FILE: umts-connection.sh
#
#       USAGE: umts-connection.sh start || stop
#
# DESCRIPTION: Script starts the UMTS Connection
#
#      AUTHOR: Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY: konzeptpark GmbH, 35633 Lahnau
#
#**********************************************************************************

PATH=/bin:/usr/bin:/sbin:/usr/sbin
. /usr/share/mcbsystools/mcblib.inc

# PID - File fuer das Skript
UMTS_CONNECTION_PID_FILE=/var/run/umts_connection.pid

#-------------------------------------------------------------------------------
function IsPPPDAlive () {
	if (pidof pppd > /dev/null) ; then
		return 1
	else
		return 0
	fi
}

function StartPPPD () {
	RefreshModemDevices
	local device=$CONNECTION_DEVICE
	syslogger "info" "UMTS-Conn - Starting pppd on device $device"
	pppd $device 460800 connect "/usr/sbin/chat -v -f /usr/share/mcbsystools/ppp-umts.chat" &
}
function StopPPPD () {
	local pids=`pidof pppd`
	syslogger "info" "UMTS-Conn - Stopping pppd ($pids)"
	if [ -z "$pids" ]; then
		syslogger "info" "$0: No pppd is running."
		return 0
	fi

	kill -TERM $pids > /dev/null
	if [ ! "$?" = "0" ]; then
		rm -f /var/run/ppp*.pid > /dev/null
	fi
}

function WaitForPPP0Device () {
	# Loop Counters
	local count_timeout=0
	local count_timeout_max=12
	local sleeptime=5
	local reached_timeout=0

	while [ true ] ; do
		if (systool -c net | grep ppp0 > /dev/null); then
			syslogger "info" "UMTS-Conn - ppp0 available"
			WriteConnectionAvailableFile
			break
		fi

		if [ $count_timeout -ge $count_timeout_max ]; then
			reached_timeout=1
			break
		fi

		syslogger "debug" "UMTS-Conn - Waiting ppp0 coming up"		
		sleep $sleeptime
		count_timeout=$[count_timeout+1]
	done

	if [ $reached_timeout -eq 1 ]; then
		return 0
	else
		return 1
	fi
}
function WaitForDataCard () {
	# Loop Counters
	local count_timeout=0
	local count_timeout_max=12
	local sleeptime=5
	local reached_timeout=0
	  
	SetSIMPIN

	#TODO: Set operator selection

	# Check for modem booked into network
	CheckNIState
	local ni_state=$?
	while [ $ni_state -ne 0 ]; do

		# Increase number of tries, when 'limited service' is reported
		# (ni_state==2) 
		if [ $ni_state -eq 2 ]; then
			count_timeout_max=18
		fi

		# Timeout ueberpruefen
		if [ $count_timeout -ge $count_timeout_max ]; then
			reached_timeout=1
			break
		fi

		sleep $sleeptime
		CheckNIState
		ni_state=$?
		count_timeout=$[count_timeout+1]

		syslogger "debug" "UMTS-Conn - Waiting for UMTS network registration ($count_timeout/$ni_state)"
	done

  if [ $reached_timeout -eq 1 ]; then
    return 1
  else
    return 0
  fi
}


#-----------------------------------------------------------------------

echo $$ > $UMTS_CONNECTION_PID_FILE

case "$1" in

	start)
		ReadModemStatus
		if 	[ $MODEM_STATUS == ${MODEM_STATES[detectedID]} ] ||
			[ $MODEM_STATUS == ${MODEM_STATES[readyID]} ] ||
			[ $MODEM_STATUS == ${MODEM_STATES[registeredID]} ]; then

			# LED 3g Timer blinken
			/usr/share/mcbsystools/leds.sh 3g timer

			WaitForDataCard
			if [ $? -eq 0 ]; then
				sleep 1
				WriteConnectionFieldStrengthFile
				WriteConnectionNetworkModeFile

				IsPPPDAlive
				if [ $? -eq 0 ]; then			
					StartPPPD
					WaitForPPP0Device
					if [ $? -eq 1 ]; then		
						WriteToModemStatusFile ${MODEM_STATES[connected]}
					else
						/usr/share/mcbsystools/leds.sh 3g off
					fi
				fi
			else
				syslogger "warn" "UMTS-Conn - Could not initialize datacard (timeout)"
				/usr/share/mcbsystools/leds.sh 3g off

				# Haben wir wirklich keine Verbindung oder liegt ein Fehler im Netz vor
				$UMTS_FS
				syslogger "info" "UMTS-Conn - Fieldstrength $?"			  
			fi		  
		fi
		rm -f $UMTS_CONNECTION_PID_FILE
		exit 0
		;;

	stop)
		StopPPPD
		CheckNIState
		rm -f $UMTS_CONNECTION_PID_FILE
		exit 0
		;;

	*)
		echo "Usage: $0 {start|stop}"
		exit 1
		;;
esac

