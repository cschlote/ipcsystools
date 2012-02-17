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
#         Die Datei "/etc/ipcsystools.conf" muss um einen Eintrag in der Sektion 
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

DESC=ipsec-monitor[$$]
STATUSFILE_DIR=/var/run
VPN_STATE_FILE=$STATUSFILE_DIR/vpn_status
RESTART_COUNT=4
CHECK_STATE_TIME=60

# only applicable when ipsec is running  
[ -e /var/run/pluto.pid ] || exit 1

# Restart the ipsec tunnel
# void RestartIPSecTunnel (char *connname)
function RestartIPSecTunnel ()
{	
  /usr/sbin/ipsec down $1 >& /dev/null	
  sleep 1
  /usr/sbin/ipsec up $1 >& /dev/null
}

# Check external IP-Addr.
# void CheckPeerAddress (void)
function CheckPeerAddress ()
{
  local timestamp=`date +%s`
  local pingaddr=$1

	# Check time intervall
  if ( test $[$timestamp - $LAST_CHECK_TIME] -gt $CHECK_STATE_TIME ); then
    # dump timestamp
    echo `date +%s` > $LAST_CHECK_FILE

    # Check external ip-addr.
    if ping -c 1 -W 10 -s 8 $pingaddr >& /dev/null ; then
		# reset ping fault counter 	
	  	CHECK_FAULT=0
      echo $CHECK_FAULT > $CHECK_FAULT_FILE	 
    else
      # inc ping fault counter
      CHECK_FAULT=$[$CHECK_FAULT+1]
      echo $CHECK_FAULT > $CHECK_FAULT_FILE
    fi
  fi
}

# Check IPSec tunnel state
# void CheckTunnelSAState (char *connname)
function CheckTunnelSAState ()
{
	local timestamp=`date +%s`

	# Check time intervall
  	if ( test $[$timestamp - $LAST_CHECK_TIME] -gt $CHECK_STATE_TIME ); then
    	# dump timestamp
    	echo `date +%s` > $LAST_CHECK_FILE

		# Check for IPSec SA
		if ipsec status | grep $1'.*STATE_QUICK_I2.*IPsec SA established' >& /dev/null ; then
			CHECK_FAULT=0
		else
			# inc ping fault counter
		  	CHECK_FAULT=$[$CHECK_FAULT+1]
		  	echo $CHECK_FAULT > $CHECK_FAULT_FILE	
		fi
	fi
}

# Process lines definition
# void ProcessPeerAddress (char *line)
ProcessPeerAddress()
{
	local line="$@"
	local loggertxt=""

	#kp-net-1722510;172.25.10.1
	local connname=$(echo $line | awk -F";" '{print $1}')
	local checkip=$(echo $line | awk -F";" '{print $2}')

	LAST_CHECK_FILE=`echo $STATUSFILE_DIR/$connname"_check_last"`
	CHECK_FAULT_FILE=`echo $STATUSFILE_DIR/$connname"_check_fault"`

	# Define check method
	if [ "$checkip" == "ipsecsa" ]; then
		check_method="sa"
	else
		check_method="ping"
	fi

	# Initialize last ping file
	if ( test ! -e $LAST_CHECK_FILE ); then
		echo `date +%s` > $LAST_CHECK_FILE
		# Fake tunnel up
		TUNNEL_UP_COUNT=$[$TUNNEL_UP_COUNT+1]
	else	
		LAST_CHECK_TIME=`cat $LAST_CHECK_FILE`

		# Read last ping fault result
		if ( test -e $CHECK_FAULT_FILE ); then
			CHECK_FAULT=`cat $CHECK_FAULT_FILE`
		else
			CHECK_FAULT=0
		fi

		# Check IP connection over icmp or SA state
		if [ "$check_method" == "sa" ]; then
			CheckTunnelSAState "$connname"
			loggertxt="IPsec SA"
		else
			CheckPeerAddress "$checkip"
			loggertxt="peer $checkip"
		fi
		
		if [ $CHECK_FAULT -gt 0 ]; then

			logger -p local0.info -t $DESC "\"$connname\": check $loggertxt - tunnel down"
	
			# Wenn n mal nicht erreicht Befehle ausführen
			if [ $CHECK_FAULT -ge $RESTART_COUNT ]; then
				logger -p local0.info -t $DESC "\"$connname\": | check $loggertxt failed $CHECK_FAULT times"
				logger -p local0.warn -t $DESC "\"$connname\": | restart tunnel..."
						
				# restart tunnel connection
				RestartIPSecTunnel "$connname"

				# reset ping fault counter
      			echo 0 > $CHECK_FAULT_FILE
			fi
		else
			# inc tunnel up counter
			TUNNEL_UP_COUNT=$[$TUNNEL_UP_COUNT+1]
			logger -p local0.info -t $DESC "\"$connname\": check $loggertxt - tunnel up"
		fi
	fi
}

# ------------------------
# --- MAIN starts here ---
#-------------------------
rc_code=0

if [ $# = 0 ]; then cmd= ; else cmd="$1"; fi
case "$cmd" in
	check)
		# set tunnel down counter
		TUNNEL_UP_COUNT=0

		monitor_peers=( `grep 'ipsec.monitorpeers' /etc/ipcsystools.conf | cut -d "=" -f2` )
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
				rc_code=0
			else
				/usr/share/mcbsystools/leds.sh vpn off
				echo "down" > $VPN_STATE_FILE
				rc_code=1
			fi
		fi
	;;
	status)
		tunnel_state=`cat $VPN_STATE_FILE`
		if [ $tunnel_state == "up" ]; then
			rc_code=0
		else
			rc_code=1
		fi
	;;
	*)	
		echo "Usage: $0 check|status"
		rc_code=1;	
	;;
esac

exit $rc_code

