
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src/commonlib -I$(top_srcdir)/src/include $(all_includes) \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	-DPKGBLDREV=\"$(strip $(shell svnversion))\" \
	@CPPFLAGS@
AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall
AM_LDFLAGS = -L$(top_srcdir)/src/commonlib -Wl,-rpath-link -Wl,$(top_srcdir)/src/commonlib/.libs/ -lbstrlib -linifile -lipctoolset

#DISTCLEANFILES = Makefile.in

## -------------------------------------------------------------------

bin_SCRIPTS =
bin_PROGRAMS = umtscardtool
sbin_PROGRAMS =

## -------------------------------------------------------------------

umtscardtool_SOURCES = umtscardtool.c setop.c fs.c ni.c pin.c customcmd.c
umtscardtool_LDFLAGS = -L$(top_srcdir)/src/commonlib -lipctoolset

install-data-hook :
	chmod +s $(DESTDIR)$(bindir)/umtscardtool

