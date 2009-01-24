## Process this file with automake to produce Makefile.in

INCLUDES =

SUBDIRS = commonlib utils etc

AM_CPPFLAGS = -DPKGLIBDIR=\"$(pkglibdir)/\" -DPKGDATADIR=\"$(pkgdatadir)/\" -DPKGBLDDATE=\"`date +%F-%R`\" -DPKGBLDREV=\"$(strip $(shell svnversion))\" @CPPFLAGS@
AM_CFLAGS = -Wall
AM_CXXFLAGS = -Wall

DISTCLEANFILES = Makefile.in


