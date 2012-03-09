## Process this file with automake to produce Makefile.in

bin_SCRIPTS = \
 ipc-umts-fs ipc-umts-op ipc-umts-ni

sbin_SCRIPTS = \
 ipc-startup ipc-shutdown

#DISTCLEANFILES = Makefile.in

nobase_dist_pkgdata_DATA = \
	options.umts \
	dip-umts.chat \
	ppp-umts.chat

nobase_pkgdata_SCRIPTS = \
	ipclib.inc \
	ipc-monitor.sh \
	connection-eth.sh \
	connection-umts.sh \
	connection-lte.sh \
	ipc-initsystem.sh \
	set_fp_leds

# Disabled for now
#	ipsec-monitor.sh \
#	msvpn_updown

man1_MANS = \
	ipc-umts-fs.man ipc-umts-op.man ipc-umts-ni.man \
	ipc-startup.man ipc-shutdown.man
