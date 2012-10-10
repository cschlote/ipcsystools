## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in

install-data-local:
#	# Install default config
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(INSTALL_DATA)  $(srcdir)/ipcsystools.conf $(DESTDIR)/etc/ipcsystools.conf
#	# Add upstart fragments to init system
	$(mkinstalldirs) $(DESTDIR)/etc/init/
#	# Upstarts doesn't behave well'
#	$(INSTALL_DATA)  $(srcdir)/init/ipc-startup.conf $(DESTDIR)/etc/init/ipc-startup.conf
#	$(INSTALL_DATA)  $(srcdir)/init/ipc-shutdown.conf $(DESTDIR)/etc/init/ipc-shutdown.conf
	$(INSTALL_DATA)  $(srcdir)/init.d/ipc-startup $(DESTDIR)/etc/init.d/ipc-startup
	$(INSTALL_DATA)  $(srcdir)/init.d/ipc-shutdown $(DESTDIR)/etc/init.d/ipc-shutdown
#	# Add startup scripts for ppp daemon
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-up.d/
	$(INSTALL_PROGRAM) $(srcdir)/if-up.d/01-3g-led-on $(DESTDIR)/etc/ppp/ip-up.d/01-3g-led-on
	$(INSTALL_PROGRAM) $(srcdir)/if-up.d/50_openvpn-up $(DESTDIR)/etc/ppp/ip-up.d/50_openvpn-up
	$(INSTALL_PROGRAM) $(srcdir)/if-up.d/50_ipsec-up $(DESTDIR)/etc/ppp/ip-up.d/50_ipsec-up
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-down.d/
	$(INSTALL_PROGRAM) $(srcdir)/if-down.d/01-3g-led-off $(DESTDIR)/etc/ppp/ip-down.d/01-3g-led-off
	$(INSTALL_PROGRAM) $(srcdir)/if-down.d/50_openvpn-down $(DESTDIR)/etc/ppp/ip-down.d/50_openvpn-down
	$(INSTALL_PROGRAM) $(srcdir)/if-down.d/50_ipsec-down $(DESTDIR)/etc/ppp/ip-down.d/50_ipsec-down
#	# Add startup scripts for ifconfig if-up/down.d
	$(mkinstalldirs) $(DESTDIR)/etc/network/if-pre-up.d/
	$(INSTALL_PROGRAM) $(srcdir)/if-pre-up.d/01-3g-led-blink $(DESTDIR)/etc/network/if-pre-up.d/01-3g-led-blink
	$(mkinstalldirs) $(DESTDIR)/etc/network/if-up.d/
	$(INSTALL_PROGRAM) $(srcdir)/if-up.d/01-3g-led-on $(DESTDIR)/etc/network/if-up.d/01-3g-led-on
	$(INSTALL_PROGRAM) $(srcdir)/if-up.d/50_openvpn-up $(DESTDIR)/etc/network/if-up.d/50_openvpn-up
	$(INSTALL_PROGRAM) $(srcdir)/if-up.d/50_ipsec-up $(DESTDIR)/etc/network/if-up.d/50_ipsec-up
	$(mkinstalldirs) $(DESTDIR)/etc/network/if-down.d/
	$(INSTALL_PROGRAM) $(srcdir)/if-down.d/01-3g-led-off $(DESTDIR)/etc/network/if-down.d/01-3g-led-off
	$(INSTALL_PROGRAM) $(srcdir)/if-down.d/50_openvpn-down $(DESTDIR)/etc/network/if-down.d/50_openvpn-down
	$(INSTALL_PROGRAM) $(srcdir)/if-down.d/50_ipsec-down $(DESTDIR)/etc/network/if-down.d/50_ipsec-down
#	# Add alias udev rules for usbmodem-ctrl and usbmodem-data
	$(mkinstalldirs) $(DESTDIR)/lib/udev/rules.d
	$(INSTALL_DATA) $(srcdir)/udev/rules.d/40-sierra-modem.rules $(DESTDIR)/lib/udev/rules.d/40-sierra-modem.rules
#	# Add cron fragment to cron.d system
	$(mkinstalldirs) $(DESTDIR)/etc/cron.d/
	$(INSTALL_DATA)  $(srcdir)/cron.d/ipcsystools $(DESTDIR)/etc/cron.d/ipcsystools
