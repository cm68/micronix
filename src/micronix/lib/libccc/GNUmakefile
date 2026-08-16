#
# cross build for libccc
#
# lib/libccc/GNUmakefile
#
# The compiler's own runtime: fmtlong and libutil, which every pass
# calls and none of them contains.  cpp, c0 and c1 compiled here long
# before this directory existed and then failed to link on _fmtlong
# and _fmtstr, because there was nothing in the tree to link them
# against.
#
# The sources live here now.  They were symlinks into ~/src/ccc/src/
# ccclib until the fork; this tree holds its own copies and is where
# they are edited.
#
# Built twice, like the passes that link it: libccc.a is the Z80
# archive, built with ccc, and libccc-host.a is the same sources
# built by the machine's own cc for the host compiler.  See
# libexec/host.mk for why both exist.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= libccc
CLASS	= lib
TARGET	= libccc.a

include $(DEPTH)/GNUmakefile.inc

HOSTCC		?= gcc
HOSTCFLAGS	?= -m32 -ggdb3 -O0 -DDEBUG -Wall -std=gnu89 -I.
#
# permalloc comes with the host archive and not the Z80 one.  It has
# one source, in libc, and a Z80 link resolves it out of libc.a - but
# the host passes have no libc.a to reach into, and every one of them
# calls it.
#
HOSTOBJS	= $(CSRCS:.c=.ho) permalloc.ho

%.ho: %.c
	$(HOSTCC) $(HOSTCFLAGS) -c $< -o $@

permalloc.ho: $(DEPTH)/lib/libc/permaloc.c
	$(HOSTCC) $(HOSTCFLAGS) -c $< -o $@

libccc-host.a: $(HOSTOBJS)
	rm -f $@
	ar cr $@ $(HOSTOBJS)

.PHONY: host hostclean
host: libccc-host.a

hostclean:
	rm -f $(HOSTOBJS) libccc-host.a
