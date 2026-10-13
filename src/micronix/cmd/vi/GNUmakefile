#
# cross build for vi
#
# cmd/vi/GNUmakefile
#
# The only command here that needs a library beyond the ones ccc links
# by itself: hexdump and logmsg come out of lib/libutil.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= vi
CLASS	= cmd

LIBS	= $(DEPTH)/lib/libutil/libutil.a

CSRCS	= cmdline.c edit.c linefunc.c main.c misccmds.c normal.c window.c

HSRCS	= stevie.h
MANSRCS	= vi.1

include $(DEPTH)/GNUmakefile.inc
