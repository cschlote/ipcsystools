#! /bin/bash

# Get default route interface
IF_EXT=`route | grep ^default | awk '{print $8}'`
if (test "$IF_EXT" == ""); then 
  IF_EXT=ppp0
fi

# Definition file for the iptables rules
FW_RULES_FILE=`grep 'save_file=' /etc/webmin/firewall/config | cut -d"=" -f2`

case "$1" in

  start)
		echo "Starting firewall..."

    # Load iptables modules
    FOUND="no"
    for LINE in `lsmod | grep ip_`
    do
	    TOK=`echo $LINE | cut -f1`
	    if [ "$TOK" = "ip_tables" ]; then
		    FOUND="yes";
		    break;
	    fi
    done
    if [ "$FOUND" = "no" ]; then
      # Module fuer die ip_tables
      modprobe ip_tables
      modprobe ip_conntrack
      modprobe iptable_nat
      modprobe ipt_MASQUERADE
      modprobe iptable_filter
      modprobe iptable_mangle
      modprobe ipt_LOG

      # FTP
      modprobe ip_nat_ftp
      modprobe ip_conntrack_ftp

      # IPSec
      modprobe ipt_ah
      modprobe ipt_esp
      modprobe ipt_tos
    fi

    # Activate IP-Forward
    echo "1" > /proc/sys/net/ipv4/ip_forward    

    # Flush all existing rules
    iptables -F && iptables -X && iptables -t nat -F && iptables -t nat -X    

		# Loading iptables settings 
		/usr/sbin/iptables-restore < $FW_RULES_FILE 2>&1
		
		# Maskarading
		if ! egrep $IF_EXT'.*MASQUERADE' $FW_RULES_FILE; then
			iptables -t nat -A POSTROUTING -o $IF_EXT -j MASQUERADE
		fi

  ;;

  stop)
    echo "Stopping firewall..."

    # Delete all existing rules 
    iptables -F
    iptables -t nat -F
  	iptables -X
  	iptables -P INPUT ACCEPT
  	iptables -P OUTPUT ACCEPT
  	iptables -P FORWARD ACCEPT
  ;;

	restart)
		$0 stop
		sleep 1
		$0 start
	;;

  *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
    ;;
esac

