#! /bin/bash

# Variablen definieren    
	IF_EXT=`route | grep ^default | awk '{print $8}'`
	if (test "$IF_EXT" == ""); then 
	  IF_EXT=ppp0
	fi

case "$1" in

  start)
    # Module laden
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
      # Module f�r die ip_tables
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

    # IP-Forward aktivieren
    echo "1" > /proc/sys/net/ipv4/ip_forward    

    # Regeln f�r die IP-Tables l�schen
    iptables -F && iptables -X && iptables -t nat -F && iptables -t nat -X

    # Gesamten NetBios traffic �ber die PPP Schnittstelle blockieren
    iptables -A FORWARD -p tcp --sport 137:139 -o $IF_EXT -j DROP
    iptables -A FORWARD -p udp --sport 137:139 -o $IF_EXT -j DROP
    iptables -A OUTPUT -p tcp --sport 137:139 -o $IF_EXT -j DROP
    iptables -A OUTPUT -p udp --sport 137:139 -o $IF_EXT -j DROP

    # Maskarading
    iptables -t nat -A POSTROUTING -o $IF_EXT -j MASQUERADE
    ;;

  stop)
    echo "Stopping firewall..."

    # Regeln f�r die IP-Tables l�schen
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


