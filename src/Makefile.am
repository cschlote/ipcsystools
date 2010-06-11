## Process this file with automake to produce Makefile.in

SUBDIRS = commonlib utils etc

AM_CPPFLAGS =  \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\"
	@CPPFLAGS@

DISTCLEANFILES = Makefile.in


