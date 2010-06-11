## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src/commonlib -I$(top_srcdir)/src/include $(all_includes) \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	@CPPFLAGS@

AM_LDFLAGS = -L$(top_srcdir)/src/commonlib -Wl,-rpath-link -Wl,$(top_srcdir)/src/commonlib/.libs/ -lbstrlib -linifile -lmcbtoolset

DISTCLEANFILES = Makefile.in

## -------------------------------------------------------------------

bin_SCRIPTS =
bin_PROGRAMS = umtscardtool
sbin_PROGRAMS =

## -------------------------------------------------------------------

umtscardtool_SOURCES = umtscardtool.c setop.c fs.c ni.c pin.c customcmd.c
umtscardtool_LDFLAGS = -L$(top_srcdir)/src/commonlib -lmcbtoolset

install-data-hook :
	chmod +s $(DESTDIR)$(bindir)/umtscardtool

