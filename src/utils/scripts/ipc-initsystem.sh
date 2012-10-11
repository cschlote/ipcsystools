#!/bin/bash
# TODO: Obsolete file! Fix ppp options issue dynamically

PATH=/sbin:/usr/sbin:/bin:/usr/bin

# Basis Bibliothek für die IPCs
#. /usr/share/ipcsystools/ipclib.inc

# Create link for Sierra Wireless Modem (classic and udev link)
( ! test -h $DESTDIR/etc/ppp/options.ttyUSB4 ) && ln -s ../../usr/share/ipcsystools/options.ppp $DESTDIR/etc/ppp/options.ttyUSB4
( ! test -h $DESTDIR/etc/ppp/options.usbmodem-data ) && ln -s ../../usr/share/ipcsystools/options.ppp $DESTDIR/etc/ppp/options.usbmodem-data

# change crontab (using cron.d only works with vixie cron not busybox)
if uname -a | grep MCB2 2>&1 >/dev/null; then
	case "$1" in
	install)
		echo -n "(check crontab) "
		entry=`crontab -l | grep ipcsystools | wc -l` 2>&1 > /dev/null
		if test $entry -eq 0; then
			( crontab -l ; echo "*/2 * * * * /usr/share/ipcsystools/ipc-cronjobs.sh" ) | crontab - 
		fi
		;;
	remove)
		crontab -l | grep -v ipcsystools | crontab -
		;;
	esac
fi

exit 0
