## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(mkinstalldirs) $(DESTDIR)/etc/mcb/
	$(mkinstalldirs) $(DESTDIR)/etc/init.d/
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-up.d/
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-down.d/
	$(INSTALL_DATA) $(srcdir)/mcbctl.conf $(DESTDIR)/etc/mcbctl.conf	
	$(INSTALL_DATA) $(srcdir)/ppp/01-3g-led-on $(DESTDIR)/etc/ppp/ip-up.d/01-3g-led-on
	$(INSTALL_DATA) $(srcdir)/ppp/01-3g-led-off $(DESTDIR)/etc/ppp/ip-down.d/01-3g-led-off
	$(INSTALL_DATA) $(srcdir)/ppp/50_openvpn-up $(DESTDIR)/etc/ppp/ip-up.d/50_openvpn-up
	$(INSTALL_DATA) $(srcdir)/ppp/50_openvpn-down $(DESTDIR)/etc/ppp/ip-down.d/50_openvpn-down
	$(INSTALL_DATA) $(srcdir)/ppp/50_ipsec-up $(DESTDIR)/etc/ppp/ip-up.d/50_ipsec-up
	$(INSTALL_DATA) $(srcdir)/ppp/50_ipsec-down $(DESTDIR)/etc/ppp/ip-down.d/50_ipsec-down

