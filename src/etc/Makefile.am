## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(INSTALL_DATA) $(srcdir)/mcbctl.conf $(DESTDIR)/etc/mcbctl.conf	
	$(mkinstalldirs) $(DESTDIR)/etc/init.d/
	$(INSTALL_DATA) $(srcdir)/init.d/mcb-startup $(DESTDIR)/etc/init.d/mcb-startup
	$(INSTALL_DATA) $(srcdir)/init.d/mcb-shutdown $(DESTDIR)/etc/init.d/mcb-shutdown
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-up.d/
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-down.d/
	$(INSTALL_DATA) $(srcdir)/ppp/01-3g-led-on $(DESTDIR)/etc/ppp/ip-up.d/01-3g-led-on
	$(INSTALL_DATA) $(srcdir)/ppp/01-3g-led-off $(DESTDIR)/etc/ppp/ip-down.d/01-3g-led-off

