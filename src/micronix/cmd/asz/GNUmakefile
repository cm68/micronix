#
# cross build for asz - the assembler.  Replaces the TRASM-derived one, now cmd/asz.old.
#
# cmd/asz/GNUmakefile
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
DIR	= asz
CLASS	= cmd

#
# wsobj.c, shared with ld and nm, lives in libutil.
#
LIBS	= $(DEPTH)/lib/libutil/libutil.a

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
# c99 and -Dlinux, not the pass defaults (gnu89, -DDEBUG).
#
# wsobj is shared; the host build compiles the copy in libutil.
HOSTSRCS = $(CSRCS) $(DEPTH)/lib/libutil/wsobj.c
HOSTCFLAGS = -m32 -std=c99 -g -O0 -Dlinux -Wall -I. -idirafter $(DEPTH)/include
HOSTPROG = mxasz
HOSTBIN  = bin

include $(DEPTH)/libexec/host.mk
