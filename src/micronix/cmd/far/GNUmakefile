#
# cross build for far
#
# cmd/far/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into far against libc and libunix.
#
# IN cmd's DIRS now, and it was not.  The reason it was out was that it
# writes a foreign diskette's directory in place and getting that wrong
# destroys it, so it wanted checking first; it has been rewritten and
# checked since, and the two lists carry it.
#
# far calls more of the C library than most commands do - malloc,
# calloc, sprintf, stat, fstat, the string calls - and TREELIBS in
# GNUmakefile.inc already names both archives, so there is nothing to
# add here.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= far
CLASS	= cmd

CSRCS	= far1.c far2.c far3.c far4.c far5.c

HSRCS	= far.h
MANSRCS	= far.1

include $(DEPTH)/GNUmakefile.inc
