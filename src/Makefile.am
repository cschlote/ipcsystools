## Process this file with automake to produce Makefile.in

SUBDIRS = lib utils etc

AM_CPPFLAGS =  \
	-DPKGLIBDIR=\"$(pkglibdir)/\" \
	-DPKGDATADIR=\"$(pkgdatadir)/\" \
	-DPKGBLDDATE=\"`date +%F-%R`\" \
	@CPPFLAGS@
AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in


