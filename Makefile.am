
SUBDIRS = src

AM_CPPFLAGS = \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
#	$(some_CFLAGS)

EXTRA_DIST = \
	autogen.sh

MAINTAINERCLEANFILES = \
	configure \
	aclocal.m4 \
	Makefile.in

