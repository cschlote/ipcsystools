
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src/include $(all_includes) \
   -DPKGLIBDIR=\"$(pkglibdir)/\" \
   -DPKGDATADIR=\"$(pkgdatadir)/\" \
   -DPKGBLDDATE=\"`date +%F-%R`\" \
   -DPKGBLDREV=\"$(strip $(shell svnversion))\" \
   @CPPFLAGS@ 
AM_CFLAGS = -Wall -Werror
AM_CXXFLAGS = -Wall -Werror
AM_LDFLAGS = -L$(top_srcdir)/src/commonlib
DISTCLEANFILES = Makefile.in

## -- Build a static/shared library for the mcbsystools binaries -------------

pkglib_LTLIBRARIES = \
 libmcbtoolset.la \
 libinifile.la \
 libbstrlib.la

libmcbtoolset_la_SOURCES = \
 lockfile.c lockfile.h \
 modem.c modem.h \
 udp.c udp.h \
 watchdog.c watchdog.h

libinifile_la_SOURCES = inifile.c inifile.h

libbstrlib_la_SOURCES = bstrlib.c bstrlib.h

