#! /bin/bash

PATH=/bin:/usr/bin:/sbin:/usr/sbin

# Basis Bibliothek für die MCB-2
#. /usr/shared/mcbsystools/mcblib.inc

# Link fuer Sierra Wireless Modem
( ! test -h /etc/ppp/options.ttyUSB4 ) && ln -s /usr/share/mcbsystools/options.umts /etc/ppp/options.ttyUSB4

# Firewall start/stop PPPD Script
[ ! -d /etc/ppp/ip-up.d/ ] && mkdir /etc/ppp/ip-up.d/
FILENAME=/etc/ppp/ip-up.d/99_firewall-up
if ( ! test -f $FILENAME ); then	
	echo "Create $FILENAME ..."
	echo "#! /bin/bash" > $FILENAME
	echo "PATH=/bin:/usr/bin:/sbin:/usr/sbin" >> $FILENAME
	echo "/usr/share/mcbsystools/firewall.sh start" >> $FILENAME
	chmod 755 $FILENAME
fi

# /etc/ppp/ip-down.d/firewall-down
[ ! -d /etc/ppp/ip-down.d/ ] && mkdir /etc/ppp/ip-down.d/
FILENAME=/etc/ppp/ip-down.d/99_firewall-down
if ( ! test -f $FILENAME ); then	
	echo "Create $FILENAME ..."
	echo "#! /bin/bash" > $FILENAME
	echo "PATH=/bin:/usr/bin:/sbin:/usr/sbin" >> $FILENAME
	echo "/usr/share/mcbsystools/firewall.sh stop" >> $FILENAME
	chmod 755 $FILENAME
fi

# crontab einrichten
# Eintrag bereits vorhanden?
entry=`crontab -l 2>/dev/null | grep checkconnection.sh | wc -l` > /dev/null
if (test $entry -eq 0); then	
	echo "Erstelle crontab Eintrag für checkconnection.sh ..."
	crontab -l 2>/dev/null /tmp/crontab.dump
	echo "*/2 * * * * /usr/share/mcbsystools/checkconnection.sh" >> /tmp/crontab.dump
	crontab /tmp/crontab.dump
	rm -f /tmp/crontab.dump
fi

