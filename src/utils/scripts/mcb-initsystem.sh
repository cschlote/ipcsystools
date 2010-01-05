#! /bin/bash

PATH=/sbin:/usr/sbin:/bin:/usr/bin

# Basis Bibliothek für die MCB-2
#. /usr/shared/mcbsystools/mcblib.inc

# Link fuer Sierra Wireless Modem
( ! test -h /etc/ppp/options.ttyUSB4 ) && ln -s /usr/share/mcbsystools/options.umts /etc/ppp/options.ttyUSB4

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

