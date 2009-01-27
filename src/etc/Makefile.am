## Process this file with automake to produce Makefile.in

INCLUDES =

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(mkinstalldirs) $(DESTDIR)/etc/mcb/
	$(mkinstalldirs) $(DESTDIR)/etc/init.d/
	$(INSTALL_DATA) mcbctl.conf $(DESTDIR)/etc/mcbctl.conf
	$(INSTALL_DATA) init.d/mcb-startup $(DESTDIR)/etc/init.d/mcb-startup
	$(INSTALL_DATA) mcb/default-sim.conf $(DESTDIR)/etc/mcb/default-sim.conf

