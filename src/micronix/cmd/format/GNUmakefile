#
# cross build for format
#
# cmd/format/GNUmakefile
#
# One program, one source file per controller: format.c has the command
# line and the label, mw.c the HD-DMA, hdca.c the HDCA, scsi.c the 5380,
# ide.c the IDE card and dj.c the DJ-DMA floppy.  The split is not
# decoration - mwformat alone was the tightest source c0 had, and the
# union of all four in one translation unit is past its heap.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= format
CLASS	= cmd

CSRCS	= format.c mw.c hdca.c scsi.c ide.c dj.c

HSRCS	= format.h

MANSRCS	= format.1

include $(DEPTH)/GNUmakefile.inc
