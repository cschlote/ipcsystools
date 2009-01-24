## Process this file with automake to produce Makefile.in

INCLUDES =

AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/
	$(INSTALL_DATA) mcbctl.conf $(DESTDIR)/etc/mcbctl.conf	
# AA	$(INSTALL_DATA) init.d/umtscontrold-initscript $(DESTDIR)/etc/umtscontrold-initscript
