#
# cross build for ccc - the driver: runs cpp, c0, c1, asz, peep and ld
#
# cmd/ccc/GNUmakefile
#
# The compiler is moving into this tree and this is the shape it has:
# one directory per pass and per tool.  The source lives here now,
# forked from ~/src/ccc/src/tools and edited here.
#
# IT IS IN cmd/GNUmakefile's DIRS NOW.  What used to be here said
# these were host programs built by gcc -m32 in the ccc tree, and that
# building them with ccc meant building the compiler for the Z80,
# which was the point of moving them and did not work yet.  It works.
#
# -DCCC IS WHAT MAKES IT THE NATIVE DRIVER, and it is the whole
# difference between the two builds of this file.  ccc.c reads
#
#	#ifdef CCC
#	    sprintf(ld_path, "%s/ld", libexecdir);
#	#else
#	    sprintf(ld_path, "%s/mxld", libexecdir);
#	#endif
#
# because on the host "ld" is the host's linker and this is not it,
# and on micronix it is the system's own.  Without the flag this
# builds a micronix binary that goes looking for a linker called mxld
# - a name that only means anything on the machine it is not running
# on.  The flag was set by hand until now, and a hand-built binary
# left in this directory was the only reason it had ever been right.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= ccc
CLASS	= cmd

CCFLAGS	= -m micronix -DCCC

include $(DEPTH)/GNUmakefile.inc

#
# and the host driver: the same source without -DCCC, so it looks for
# mxld rather than ld - on this machine "ld" is the host's linker and
# emphatically not ours.  It goes in bin and not libexec, being the
# one part of the compiler a person types.
#
HOSTPROG	= mxccc
HOSTBIN		= bin
HOSTDEFS	=
HOSTWARNS	= -Wall
HOSTCFLAGS	= -m32 $(HOSTDEBUG) $(HOSTWARNS) -I. -I$(DEPTH)/lib/libccc

include $(DEPTH)/libexec/host.mk
