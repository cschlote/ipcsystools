#!/bin/bash
#***********************************************************************************
#
#        FILE:  ipsec-monitor.sh
#
# DESCRIPTION:  Monitoring IP-Addr. for ipsec tunnels
#
#      AUTHOR:  Dipl. Math. (FH) Andreas Ascheneller, a.ascheneller@konzeptpark.de
#     COMPANY:  konzeptpark GmbH, 35633 Lahnau
#
#       USAGE:  
#
#         Die Datei "/etc/mcbctl.conf" muss um einen Eintrag in der Sektion 
#         #[WATCHDOG-VPN]
#         "vpn.monitorpeers=..." 
#         erweitert werden. In dem Eintrag stehen dann die Peer Informationen.
#    
#         Aufbau
#         ------
#           Zupruefende Connection, IP-Adresse zur Ueberwachtung
#              
#         Beispiel
#         --------
#           In der ipsec.conf existiert eine "connection": conn kp-net-1722510
#           => vpn.monitorpeers=kp-net-1722510;172.25.10.1
#           Mehrere Eintrag werden durch Leerzeichen getrennt.
#           vpn.monitorpeers=kp-net-1722510;172.25.10.1 kp-net-1722520;172.25.20.1
#
#***********************************************************************************

DESC=ipsec-monitor
STATUSFILE_DIR=/var/run
VPN_STATE_FILE=$STATUSFILE_DIR/vpn_status
RESTART_COUNT=4
CHECK_PING_TIME=60

# only applicable when ipsec is running  
[ -e /var/run/pluto.pid ] || exit

# Restart the ipsec tunnel
# void RestartIPSecTunnel (char *connname)
function RestartIPSecTunnel ()
{	
	/usr/sbin/ipsec down $1	>& /dev/null	
	sleep 1
	/usr/sbin/ipsec up $1	>& /dev/null	
}

# Check external IP-Addr.
# void CheckPeerAddress (void)
function CheckPeerAddress ()
{
  local timestamp=`date +%s`

	# Check time intervall
  if ( test $[$timestamp - $LAST_PING_TIME] -gt $CHECK_PING_TIME ); then

    # dump timestamp
    echo `date +%s` > $LAST_PING_FILE

    # Check external ip-addr.
    if ping -c 1 -W 10 -s 8 $CHECK_PING_IP >& /dev/null ; then
			# reset ping fault counter 	
	  	PING_FAULT=0
      echo $PING_FAULT > $PING_FAULT_FILE	 
    else
      # inc ping fault counter
      PING_FAULT=$[$PING_FAULT+1]
      echo $PING_FAULT > $PING_FAULT_FILE
    fi
  fi
}

# Process lines definition
# void ProcessPeerAddress (char *line)
ProcessPeerAddress()
{
	local line="$@"

	#kp-net-1722510;172.25.10.1
	local connname=$(echo $line | awk -F";" '{print $1}')
	local checkip=$(echo $line | awk -F";" '{print $2}')

	LAST_PING_FILE=`echo $STATUSFILE_DIR/$connname"_last_ping"`
	PING_FAULT_FILE=`echo $STATUSFILE_DIR/$connname"_ping_fault"`
	CHECK_PING_IP=$checkip

	# Initialize last ping file
	if ( test ! -e $LAST_PING_FILE ); then
		echo `date +%s` > $LAST_PING_FILE
		# Fake tunnel up
		TUNNEL_UP_COUNT=$[$TUNNEL_UP_COUNT+1]
	else	
		LAST_PING_TIME=`cat $LAST_PING_FILE`

		# Read last ping fault result
		if ( test -e $PING_FAULT_FILE ); then
			PING_FAULT=`cat $PING_FAULT_FILE`
		else
			PING_FAULT=0
		fi

		# Check IP connection over icmp
		CheckPeerAddress
		if [ $PING_FAULT -gt 0 ]; then
			
			logger -p local0.info -t $DESC "\"$connname\": check peer $checkip, tunnel down"
	
			# Wenn n mal nicht erreicht Befehle ausführen
			if [ $PING_FAULT -ge $RESTART_COUNT ]; then
				logger -p local0.info -t $DESC "\"$connname\": | check peer failed $PING_FAULT times"
				logger -p local0.warn -t $DESC "\"$connname\": | restart tunnel..."
						
				# restart tunnel connection
				RestartIPSecTunnel "$connname"

				# reset ping fault counter
      	echo 0 > $PING_FAULT_FILE
			fi
		else
			# inc tunnel up counter
			TUNNEL_UP_COUNT=$[$TUNNEL_UP_COUNT+1]
			logger -p local0.info -t $DESC "\"$connname\": check peer $checkip, tunnel up"
		fi
	fi
}

# ------------------------
# --- MAIN starts here ---
#-------------------------

# set tunnel down counter
TUNNEL_UP_COUNT=0

monitor_peers=( `grep 'vpn.monitorpeers' /etc/mcbctl.conf | cut -d "=" -f2` )
#echo ${monitor_peers[@]}

if ( test ${#monitor_peers[*]} -gt 0 ); then
	# Check peeraddresses
	for peer in "${monitor_peers[@]}"
	do 	
		ProcessPeerAddress $peer
	done

	# Turn on/of VPN LED
	if ( test $TUNNEL_UP_COUNT -gt 0 ); then
		/usr/share/mcbsystools/leds.sh vpn on
		echo "up" > $VPN_STATE_FILE
	else
		/usr/share/mcbsystools/leds.sh vpn off
		echo "down" > $VPN_STATE_FILE
	fi
fi

exit 0

