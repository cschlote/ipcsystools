#
# automake config
#
AM_CPPFLAGS = \
	-I$(top_srcdir)/src/commonlib \
	-I$(top_srcdir)/src/include $(all_includes) \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	@CPPFLAGS@
AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

AM_LDFLAGS = -Wl,-rpath-link -Wl,$(lib_srcdir) 

MAINTAINERCLEANFILES = \
	Makefile.in

SUBDIRS = \
 scripts

bin_PROGRAMS = \
	binblob

binblob_SOURCES = binblob.c
binblob_LDFLAGS = -L$(top_builddir)/src/commonlib -L$(top_builddir)/src/commonlib/.libs
binblob_LDADD = -lbinblob

