
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src

AM_CPPFLAGS = \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	-DPKGBLDREV=\"$(strip $(shell svnversion))\" \
	@CPPFLAGS@

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

