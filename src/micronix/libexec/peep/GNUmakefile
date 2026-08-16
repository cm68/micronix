#
# cross build for peep - the peephole optimiser, run over c1's output for -O.
#
# cmd/peep/GNUmakefile
#
# PROTOTYPE.  The compiler is moving into this tree, and this is the
# shape it will have when it arrives: one directory per pass and per
# tool.  The sources live here.  They were symlinks into ~/src/ccc until
# the fork; this tree holds its own copies and is where they are
# edited now.
#
# Not in cmd/GNUmakefile's DIRS.  These are host programs today, built
# by gcc -m32 in the ccc tree; building them here with ccc means
# building the compiler for the Z80, which is the point of moving them
# but is not something that works yet.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= peep
CLASS	= libexec

#
# Not /bin.  ccc runs this; nobody types it.
#
BINDIR	= /libexec

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
#
HOSTDEFS =
HOSTCFLAGS = -m32 $(HOSTDEBUG) $(HOSTWARNS) -I. -I$(DEPTH)/lib/libccc
HOSTPROG = mxpeep

include $(DEPTH)/libexec/host.mk
