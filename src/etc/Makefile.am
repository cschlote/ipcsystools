## Process this file with automake to produce Makefile.in

INCLUDES =

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(mkinstalldirs) $(DESTDIR)/etc/mcb/
	$(mkinstalldirs) $(DESTDIR)/etc/init.d/
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-up.d/
	$(mkinstalldirs) $(DESTDIR)/etc/ppp/ip-down.d/
	$(INSTALL_DATA) mcbctl.conf $(DESTDIR)/etc/mcbctl.conf	
	$(INSTALL_DATA) ppp/01-3g-led-on $(DESTDIR)/etc/ppp/ip-up.d/01-3g-led-on
	$(INSTALL_DATA) ppp/01-3g-led-off $(DESTDIR)/etc/ppp/ip-down.d/01-3g-led-off

