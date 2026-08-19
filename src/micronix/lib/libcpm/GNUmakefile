#
# libcpm - the CP/M system call library, and crtcpm to start it
#
# lib/libcpm/GNUmakefile
#
# The compiler targets two systems.  Micronix is the one it runs on
# and libu is that system's call layer; CP/M is the other, and this
# is that one - a program built with "ccc -m cpm" is linked against
# libcpm.a and started by crtcpm.o instead of libu.a and crt0.o.
#
# The driver chooses between them, so nothing in a source has to: -m
# picks the runtime, the include path and the target flag, and the
# headers a program includes are the same headers either way.  That
# is what makes the C library's set unambiguous - the ambiguity that
# would otherwise live in the sources lives in one place, in ccc.c.
#
# THE POINT IS CROSS COMPILATION FROM MICRONIX.  The compiler runs on
# micronix and can produce a CP/M object there, which is why this has
# to be in this tree and built by this tree's compiler rather than
# borrowed from the host.
#
# crtcpm.o is not in the archive.  It is the startup object, named by
# the driver on the link line, and an archive member is only pulled
# in to satisfy a reference - which nothing makes to a start symbol.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

#
# -m cpm, because this is the CP/M runtime.  The default is micronix,
# and building the CP/M library for micronix is a contradiction that
# links and then does the wrong system calls.
#
CCFLAGS	= -m cpm -O

DEPTH	= ../..
DIR	= libcpm
CLASS	= lib
TARGET	= libcpm.a

#
# getargs.s did not come across from ccc's copy of this library.  It
# was compiler output left beside getargs.c - "ld hl,(iy+4)", which
# is not a Z80 instruction - and since %.o from %.s is preferred to
# %.o from %.c, make built the stale one and the assembler refused
# it.  Filtering it out of ASSRCS does not help: the rule matches the
# file, not the list.
#
#
# fakecpcl.s is the OTHER _cpm_clean: a bare ret, for a program that
# wants no file cleanup at exit and would rather not drag the _fcb
# table in with it.  cleanup.c is the real one.  Both in the archive
# is a duplicate symbol as soon as anything exits, so the stub stays
# out of it - a program that wants it names the object.
#
CSRCS	= $(filter-out crtcpm.c, $(wildcard *.c))
ASSRCS	= $(filter-out crtcpm.s fakecpcl.s, $(wildcard *.s))
TARGETS	= crtcpm.o fakecpcl.o

include $(DEPTH)/GNUmakefile.inc

#
# The CP/M runtime goes under /lib/cpm, not /lib beside libu.a, so a
# target directory is self-contained.  Point LIBINSTDIR at that tree
# and the archive install below puts libcpm.a there; crtcpm.o and the
# CP/M copy of the (target-independent) libc.a are not the archive
# target, so installcpm ships them.
#
LIBINSTDIR = $(ROOT)/lib/cpm

install: installcpm

installcpm: crtcpm.o
	mkdir -p $(LIBINSTDIR)
	cp crtcpm.o $(LIBINSTDIR)/crtcpm.o
	cp ../libc/libc.a $(LIBINSTDIR)/libc.a
