#
# cross build for far
#
# cmd/far/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into far against libc and libu.
#
# Not in cmd's DIRS, for the reason mkfs is not: this writes a foreign
# diskette's directory in place, and getting that wrong destroys it.
# It wants checking before it is built by default.
#
# far calls more of the C library than most commands do - malloc,
# calloc, sprintf, stat, fstat, the string calls - and TREELIBS in
# GNUmakefile.inc already names both archives, so there is nothing to
# add here.  It is not in DIRS, so it is built by hand:
#
#	cd cmd/far && make
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= far
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc
