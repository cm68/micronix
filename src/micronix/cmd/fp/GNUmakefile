#
# cross build for fp
#
# cmd/fp/GNUmakefile
#
# The recovered Whitesmith's C source, ported to the reconstruction:
# every .c in this directory, linked into fp.
#
# fp.dist (the original binary) and fp.dis (its disassembly) are kept
# here as historical artifacts; the port is from the recovered source,
# not from the disassembly.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= fp
CLASS	= cmd

CSRCS	= acct.c change.c copy.c device.c dir.c display.c error.c \
	 extract.c file.c format.c global.c incr.c init.c label.c link.c main.c \
	 mkdir.c replace.c sum.c super.c table.c

HSRCS	= dtab.h fp.h
MANSRCS	= fp.1

include $(DEPTH)/GNUmakefile.inc
