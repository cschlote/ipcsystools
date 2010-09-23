#
# automake config
#
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
#	$(some_CFLAGS)

MAINTAINERCLEANFILES = \
	Makefile.in

SUBDIRS = \
 scripts

bin_PROGRAMS = \
	binblob

binblob_SOURCES = binblob.c

binblob_LDADD = -lbinblob

