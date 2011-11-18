## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

#DISTCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(mkinstalldirs) $(DESTDIR)/etc/init/
	$(INSTALL_DATA)  $(srcdir)/init/ipc-startup.conf $(DESTDIR)/etc/init/ipc-startup.conf
	$(INSTALL_DATA)  $(srcdir)/init/ipc-shutdown.conf $(DESTDIR)/etc/init/ipc-shutdown.conf
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-up.d/
	$(INSTALL_DATA) $(srcdir)/ppp/ip-up.d/01-3g-led-on $(DESTDIR)/etc/ppp/ip-up.d/01-3g-led-on
	$(INSTALL_DATA) $(srcdir)/ppp/ip-up.d/50_openvpn-up $(DESTDIR)/etc/ppp/ip-up.d/50_openvpn-up
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-down.d/
	$(INSTALL_DATA) $(srcdir)/ppp/ip-down.d/01-3g-led-off $(DESTDIR)/etc/ppp/ip-down.d/01-3g-led-off
	$(INSTALL_DATA) $(srcdir)/ppp/ip-down.d/50_openvpn-down $(DESTDIR)/etc/ppp/ip-down.d/50_openvpn-down
	$(mkinstalldirs) $(DESTDIR)/etc/udev/rules.d
	$(INSTALL_DATA) $(srcdir)/udev/rules.d/10-ncu-firmware.rules $(DESTDIR)/etc/udev/rules.d/10-ncu-firmware.rules

# Replaced by scripts in /sbin
#	$(mkinstalldirs) $(DESTDIR)/etc/init.d/
#	$(INSTALL_DATA) $(srcdir)/init.d/ipc-startup $(DESTDIR)/etc/init.d/ipc-startup
#	$(INSTALL_DATA) $(srcdir)/init.d/ipc-shutdown $(DESTDIR)/etc/init.d/ipc-shutdown

# Not yet used???
#	#$(INSTALL_DATA) $(srcdir)/ppp/50_ipsec-up $(DESTDIR)/etc/ppp/ip-up.d/50_ipsec-up
#	#$(INSTALL_DATA) $(srcdir)/ppp/50_ipsec-down $(DESTDIR)/etc/ppp/ip-down.d/50_ipsec-down	
#	#$(INSTALL_DATA) $(srcdir)/mcbctl.conf $(DESTDIR)/etc/mcbctl.conf	
