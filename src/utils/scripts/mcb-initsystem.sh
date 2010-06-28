#!/bin/bash
# This script is called by the installer script. Do not delete.

PATH=/sbin:/usr/sbin:/bin:/usr/bin

#. /usr/share/mcbsystools/mcblib.inc

#
# Create link for Sierra Wireless Modem ppp options file 
#
if [ ! -L /etc/ppp/options.ttyUSB4 ]; then
	rm -f /etc/ppp/options.ttyUSB4
	ln -s /usr/share/mcbsystools/options.umts /etc/ppp/options.ttyUSB4
	echo "Setup modem options of PPPD"
fi

#
# Add checkconnection.sh to system cron table
#
entry=`crontab -l 2>/dev/null | grep checkconnection.sh | wc -l` > /dev/null
if [ $entry -eq 0 ]; then	
	echo "Creating crontab entry for checkconnection.sh ..."
	crontab -l 2>/dev/null /tmp/crontab.dump
	echo "*/2 * * * * /usr/share/mcbsystools/checkconnection.sh" >> /tmp/crontab.dump
	crontab /tmp/crontab.dump
	rm -f /tmp/crontab.dump
fi

