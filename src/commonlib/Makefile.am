
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src/include $(all_includes) \
   -DPKGLIBDIR=\"$(pkglibdir)/\" \
   -DPKGDATADIR=\"$(pkgdatadir)/\" \
   -DPKGBLDDATE=\"`date +%F-%R`\" \
   -DPKGBLDREV=\"$(strip $(shell git describe))\" \
   @CPPFLAGS@ 
AM_CFLAGS = -Wall -Werror
AM_CXXFLAGS = -Wall -Werror
AM_LDFLAGS = -L$(top_srcdir)/src/commonlib

DISTCLEANFILES = Makefile.in

## -- Build a static/shared library for the ipcsystools binaries ------

#lib_LTLIBRARIES = \
# libbinblob.la

libbinblob_la_SOURCES = libbinblob.c libbinblob.h

pkglib_LTLIBRARIES = \
 libipctoolset.la \
 libinifile.la \
 libbstrlib.la \
 libbinblob.la

libipctoolset_la_SOURCES = \
 lockfile.c lockfile.h \
 modem.c modem.h \
 udp.c udp.h \
 watchdog.c watchdog.h

libinifile_la_SOURCES = inifile.c inifile.h

libbstrlib_la_SOURCES = bstrlib.c bstrlib.h

