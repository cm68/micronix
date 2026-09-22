#
# cross build for asz - the assembler.  Replaces the TRASM-derived one, now cmd/asz.old.
#
# cmd/asz/GNUmakefile
#
# Built twice from the one set of sources.  The framework below builds
# it WITH ccc, FOR the Z80, which is the assembler that runs on micronix
# and the one the driver execs beside itself in bin.  host.mk at the
# foot builds the same sources with gcc -m32, to run here - that is
# mxasz, the assembler the cross build needs.
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

#
# obj.h is the object-format header, in the tree's include/, reached
# with the -idirafter above since the host has none of its own.  It is
# not in HSRCS - that is $(wildcard *.h) here - so editing it would
# rebuild nothing; make the host objects depend on it.
#
$(HOSTOBJS): $(DEPTH)/include/obj.h
