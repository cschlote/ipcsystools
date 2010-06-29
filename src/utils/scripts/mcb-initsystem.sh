#!/bin/bash
# This script is called by the installer script. Do not delete.

PATH=/sbin:/usr/sbin:/bin:/usr/bin

#. /usr/share/mcbsystools/mcblib.inc

case "$1" in
	"install")
		#
		# Create link for Sierra Wireless Modem ppp options file 
		#
		echo "Checking modem options of PPPD..."
		if [ ! -L $DESTDIR/etc/ppp/options.ttyUSB4 ]; then
			rm -f $DESTDIR/etc/ppp/options.ttyUSB4
			ln -sf /usr/share/mcbsystools/options.umts $DESTDIR/etc/ppp/options.ttyUSB4
			echo "Setup modem options of PPPD"
			echo "at $DESTDIR/etc/ppp/options.ttyUSB4"
		fi

		#
		# Add checkconnection.sh to system cron table
		#
		if [ -z "$DESTDIR" ]; then
			echo "Checking root crontab entries..."

			entry=`crontab -l 2>/dev/null | grep checkconnection.sh | wc -l` > /dev/null
			if [ $entry -eq 0 ]; then	
				echo "Creating crontab entry for checkconnection.sh ..."
				crontab -l 2>/dev/null /tmp/crontab.dump
				echo "*/2 * * * * /usr/share/mcbsystools/checkconnection.sh" >> /tmp/crontab.dump
				crontab /tmp/crontab.dump
				rm -f /tmp/crontab.dump
			fi
		else
			echo "(cross-install) Checking root crontab entries..."
			
			entry=`cat $DESTDIR/var/spool/cron/crontabs/root 2>/dev/null | grep checkconnection.sh | wc -l` > /dev/null
			if [ $entry -eq 0 ]; then	
				echo "(cross-install) Creating crontab entry for checkconnection.sh ..."
				echo "*/2 * * * * /usr/share/mcbsystools/checkconnection.sh" >> $DESTDIR/var/spool/cron/crontabs/root
			fi
		fi
	;;

	"remove")
		#
		# Remove link for Sierra Wireless Modem ppp options file 
		#
		echo "Remove links to modem options of PPPD..."
		if [ -L $DESTDIR/etc/ppp/options.ttyUSB4 ]; then
			rm -f $DESTDIR/etc/ppp/options.ttyUSB4
		fi

		#
		# Remove checkconnection.sh to system cron table
		#
		echo "Removing root crontab entries..."
		if [ -z "$DESTDIR" ]; then

			entry=`crontab -l 2>/dev/null | grep checkconnection.sh | wc -l` > /dev/null
			if [ $entry -ne 0 ]; then	
				echo "Removing crontab entry for checkconnection.sh ..."
				crontab -l 2>/dev/null | grep -v "checkconnection.sh" | crontab
			fi
		else
			entry=`cat $DESTDIR/var/spool/cron/crontabs/root 2>/dev/null | grep checkconnection.sh | wc -l` > /dev/null
			if [ $entry -ne 0 ]; then	
				echo "(cross-install) Removing crontab entry for checkconnection.sh ..."
				grep -v "checkconnection.sh" $DESTDIR/var/spool/cron/crontabs/root > /tmp/crontab.temp
				mv /tmp/crontab.temp $DESTDIR/var/spool/cron/crontabs/root
			fi
		fi
	;;
esac

exit 0
