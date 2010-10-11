## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/commonlib \
	-I$(top_srcdir)/src/include $(all_includes) \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	@CPPFLAGS@
AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

AM_LDFLAGS = -Wl,-rpath-link -Wl,$(pkglib_srcdir) 

DISTCLEANFILES = Makefile.in

## -------------------------------------------------------------------

bin_SCRIPTS =
bin_PROGRAMS = umtscardtool fs_ping
sbin_PROGRAMS =

## -------------------------------------------------------------------

umtscardtool_SOURCES = umtscardtool.c setop.c fs.c ni.c pin.c customcmd.c
umtscardtool_LDFLAGS = -L$(top_builddir)/src/commonlib -L$(top_builddir)/src/commonlib/.libs -lmcbtoolset

fs_ping_SOURCES = fs_ping.c
fs_ping_LDFLAGS = -L$(top_builddir)/src/commonlib -L$(top_builddir)/src/commonlib/.libs -lmcbtoolset

install-data-hook :
	chmod +s $(DESTDIR)$(bindir)/umtscardtool

