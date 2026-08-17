#
# cross build for size - reports the segment sizes of an object.
#
# cmd/size/GNUmakefile
#
# size is in cmd/GNUmakefile's DIRS and cross builds the micronix
# command with ccc, like everything else here.  It is also a host
# tool: mxsize, the same program compiled for this machine, which is
# what reads a micronix object's header on the build host.  mxar and
# mxnm are built at the top of the tree; size builds its own here,
# because it needs nothing but its own source, and installs into
# /usr/local/bin where the compiler and the other mx tools live.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= size
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

#
# The host build.  size.c is the same source the cross build compiles;
# this compiles it with the machine's own cc.  -std=gnu89 is the C the
# source is written in - it has to compile with ccc as well, which has
# no later dialect.  -Dlinux selects the host's headers (stdlib, string).
#
HOSTCC		?= gcc
HOSTCFLAGS	?= -w -std=gnu89 -Dlinux
MXSIZE		 = $(BIN)/mxsize

$(MXSIZE): size.c wsobj.h
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ size.c

#
# The host tool is a prerequisite of all, so a fresh tree builds it
# beside the Z80 one without being asked.
#
all: $(MXSIZE)

#
# Install the host tool into /usr/local/bin, where ccc and the other
# mx tools live, so "mxsize" runs without naming the tree.  /usr/local
# is not ours; sudo, and SUDO= for a DESTDIR you already own.
#
PREFIX	?= /usr/local
SUDO	?= sudo

sysinstall: $(MXSIZE)
	$(SUDO) mkdir -p $(PREFIX)/bin
	$(SUDO) cp $(MXSIZE) $(PREFIX)/bin/

.PHONY: sysinstall
