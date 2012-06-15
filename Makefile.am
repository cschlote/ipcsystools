
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src

AM_CPPFLAGS = \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	@CPPFLAGS@

EXTRA_DIST = \
	autogen.sh

DISTCLEANFILES = \
	Makefile.in

MAINTAINERCLEANFILES = \
	configure \
	aclocal.m4 \
	Makefile.in

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

