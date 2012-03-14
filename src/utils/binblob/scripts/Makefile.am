## Process this file with automake to produce Makefile.in

bin_SCRIPTS = \
	ipc-getfirmware \
	ipc-getgpslatitude \
	ipc-getgpslongitude \
	ipc-getgpssatinfo \
	ipc-getgpstime \
	ipc-getgsmcellid \
	ipc-getgsmfs \
	ipc-getgsmlac \
	ipc-getgsmmode \
	ipc-getgsmplmn \
	ipc-getgsmroaming \
	ipc-getipdez \
	ipc-gettemp      

DISTCLEANFILES = Makefile.in

nobase_dist_pkgdata_DATA = \
	asciimsg.script \
	mcb2msg1.script \
	mcb2msg2.script \
	mcb2msg3.script \
	mcb2msg4.script \
	mcb2msg5.script

man1_MANS = \
	$(addsuffix .man,$bin_SCRIPTS)

#devel:	statusmsg.script 
