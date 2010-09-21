#
# automake config
#
SUBDIRS = \
 scripts

bin_PROGRAMS = \
	binblob

lib_LTLIBRARIES = \
	libbinblob.la

dist_bin_SCRIPTS = \
	binblob


AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
#	$(some_CFLAGS)


MAINTAINERCLEANFILES = \
	configure \
	aclocal.m4 \
	Makefile.in

#
# library
#
libbinblob_la_SOURCES = \
	libbinblob.c

#
# binary
#
binblob_SOURCES = \
	binblob.c

binblob_LDADD = \
	libbinblob.la
#	$(some_LIBS)

#binblob_SOURCES = \
#	binblob.sh

