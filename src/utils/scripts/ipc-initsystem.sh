#!/bin/bash
# TODO: Obsolete file! Fix ppp options issue dynamically

PATH=/sbin:/usr/sbin:/bin:/usr/bin

# Basis Bibliothek für die IPCs
#. /usr/share/ipcsystools/ipclib.inc

# Create link for Sierra Wireless Modem
( ! test -h $DESTDIR/etc/ppp/options.ttyUSB4 ) && ln -s $DESTDIR/usr/share/ipcsystools/options.umts $DESTDIR/etc/ppp/options.ttyUSB4

# change crontab (obsolete - using cron.d)
#entry=`crontab -l 2>/dev/null | grep ipc-monitor.sh | wc -l` > /dev/null
#if (test $entry -eq 0); then
#	echo "Creating crontab entry for ipc-monitor.sh ..."
#	crontab -l 2>/dev/null /tmp/crontab.dump
#	echo "*/2 * * * * /usr/share/ipcsystools/ipc-monitor.sh" >> /tmp/crontab.dump
#	crontab /tmp/crontab.dump
#	rm -f /tmp/crontab.dump
#fi

exit 0
