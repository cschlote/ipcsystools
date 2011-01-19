#! /bin/bash

PATH=/sbin:/usr/sbin:/bin:/usr/bin

# Basis Bibliothek für die MCB-2
#. /usr/shared/mcbsystools/mcblib.inc

# Create link for Sierra Wireless Modem
( ! test -h /etc/ppp/options.ttyUSB4 ) && ln -s /usr/share/mcbsystools/options.umts /etc/ppp/options.ttyUSB4

# change crontab
# Check crontab entry
entry=`crontab -l 2>/dev/null | grep mcb-monitor.sh | wc -l` > /dev/null
if (test $entry -eq 0); then
	echo "Creating crontab entry for mcb-monitor.sh ..."
	crontab -l 2>/dev/null /tmp/crontab.dump
	echo "*/2 * * * * /usr/share/mcbsystools/mcb-monitor.sh" >> /tmp/crontab.dump
	crontab /tmp/crontab.dump
	rm -f /tmp/crontab.dump
fi

