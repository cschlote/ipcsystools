## Process this file with automake to produce Makefile.in

bin_SCRIPTS = \
	ipc-umts-fs ipc-umts-op ipc-umts-ni

sbin_SCRIPTS = \
	gps-monitor \
	ipc-monitor \
	ipc-reportstatus \
	ipsec-monitor \
	ipc-startup ipc-shutdown ipc-control

DISTCLEANFILES = Makefile.in

nobase_dist_pkgdata_DATA = \
	options.ppp

nobase_pkgdata_SCRIPTS = \
	ipclib.inc \
	connection-dip.sh \
	connection-eth.sh \
	connection-ppp.sh \
	connection-pppoe.sh \
	ipc-initsystem.sh \
	ipc-cronjobs.sh \
	set_fp_leds

# Disabled for now
#	msvpn_updown

man1_MANS = \
	$(addsuffix .man,$(bin_SCRIPTS)) \
	$(addsuffix .man,$(sbin_SCRIPTS))
