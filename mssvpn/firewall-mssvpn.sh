#! /bin/bash

# Define variables
	IF_EXT=`route | grep ^default | awk '{print $8}'`
	if (test "$IF_EXT" == ""); then 
	  IF_EXT=ppp0
	fi

case "$1" in

  start)
    # Load modules
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
      # ip_tables
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

    # Delete existing ip-tables rules
    iptables -F && iptables -X && iptables -t nat -F && iptables -t nat -X

    # Block NetBios traffic for PPP interface
    iptables -A FORWARD -p tcp --sport 137:139 -o $IF_EXT -j DROP
    iptables -A FORWARD -p udp --sport 137:139 -o $IF_EXT -j DROP
    iptables -A OUTPUT -p tcp --sport 137:139 -o $IF_EXT -j DROP
    iptables -A OUTPUT -p udp --sport 137:139 -o $IF_EXT -j DROP

		# Right IPSec Subnet
		IPSEC_NET=`grep "rightsubnet=.*/22" /etc/ipsec.conf | cut -d "=" -f2`		

		# For the MTU Problem
		iptables -I FORWARD -o $IF_EXT -p tcp --tcp-flags SYN,RST SYN -d $IPSEC_NET -j TCPMSS --set-mss 1330

    # Masquarading
		iptables -t nat -A POSTROUTING -o $IF_EXT ! -d $IPSEC_NET -j MASQUERADE
    ;;

  stop)
    echo "Stopping firewall..."

    # Delete existing ip-tables rules
    iptables -F
    iptables -t nat -F
  	iptables -X
  	iptables -P INPUT ACCEPT
  	iptables -P OUTPUT ACCEPT
  	iptables -P FORWARD ACCEPT
    ;;

  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac


