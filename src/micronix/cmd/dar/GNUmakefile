#
# cross build for dar
#
# cmd/dar/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into dar.
#
# dar.ctl and dar.dist are data, not sources.  dar.dis is generated
# from them by "make disas" and is checked in so the reconstruction can
# be read against it without needing the tool.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= dar
CLASS	= cmd

CSRCS	= compat.c dar2.c dar3.c dar4.c dar5.c dar6.c delete.c dev.c dir.c \
	 error.c extract.c fat.c fs.c global.c init.c mkdir.c name.c path.c \
	 replace.c string.c table.c time.c unix.c

HSRCS	= dar.h

include $(DEPTH)/GNUmakefile.inc

DISAS	= $(DEPTH)/../tools/disas

dar.dis: dar.ctl dar.dist
	$(DISAS) -f dar.ctl dar.dist > $@
