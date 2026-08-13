#
# cross build for ccc - the driver: runs pass0, c0, c1, asz and wsld in turn.
#
# cmd/ccc/GNUmakefile
#
# PROTOTYPE.  The compiler is moving into this tree, and this is the
# shape it will have when it arrives: one directory per pass and per
# tool.  The sources here are symlinks into ~/src/ccc/src/tools, which is
# still where they are edited, so nothing is duplicated while both
# trees exist.
#
# Not in cmd/GNUmakefile's DIRS.  These are host programs today, built
# by gcc -m32 in the ccc tree; building them here with ccc means
# building the compiler for the Z80, which is the point of moving them
# but is not something that works yet.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= ccc
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc
