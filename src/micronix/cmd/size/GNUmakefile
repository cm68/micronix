#
# cross build for size - reports the segment sizes of an object.
#
# cmd/size/GNUmakefile
#
# size is in cmd/GNUmakefile's DIRS and cross builds the micronix
# command with ccc, like everything else here.  Its host build, mxsize
# - the same source compiled for this machine, which is what reads a
# micronix object's header on the build host - is at the top of the
# tree with mxar and mxnm, not here; see "size" in ../../GNUmakefile.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= size
CLASS	= cmd

CSRCS	= size.c

MANSRCS	= size.1

include $(DEPTH)/GNUmakefile.inc
