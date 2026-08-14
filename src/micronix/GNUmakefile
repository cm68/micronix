#
# top of the micronix cross build
#
# /GNUmakefile
#
# This drives the cross build only.  The "makefile" beside it is the
# native build that runs inside the simulator.
#
# hitech and whitesmith are deliberately not here.  Neither is part of
# the build any more: the compiler that builds this tree is ccc, and
# it is installed in /usr/local.  Their sources are still in the tree,
# with their own makefiles, and are built by hand if they are built at
# all.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# include is here now.  It was not, and nothing in the cross build
# looked at that directory at all: the headers in /usr/src/include
# were whatever had been copied there in 2023, and four of them had
# drifted from the tree.  It goes FIRST - what everything else
# compiles against is installed before anything compiles.
#
DIRS = include lib libexec cmd sys

#
# "cd $$d &&", not "cd $$d ;" - see the note in GNUmakefile.inc.
#
RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

#
# Put the system headers where the compiler looks for them.
#
# ccc resolves <sys/...> out of its system include directory - libdir
# up one and into usr/include, see tools/ccc.c - and that search runs
# before -I.  These are the headers that directory should be holding:
# sys/stat.h, sys/fs.h, sys/dir.h, types.h and the rest are the
# system's, and the system is this tree.  Until they are installed,
# every command that stats a file has to be told where to look.
#
# sudo, because $(PREFIX) is not ours.  SUDO= runs it without, for a
# DESTDIR you already own.
#
PREFIX	?= /usr/local
SYSINC	 = $(DESTDIR)$(PREFIX)/usr/include
SUDO	?= sudo

#
# NOT copied: the headers ccc installs under these names.  A command
# links the libc built in lib/libc, and that libc is compiled against
# ccc's stdio.h; this tree's stdio.h is the old Whitesmith one and its
# FILE is a different shape.  Since the system include directory is
# searched first, copying ours over ccc's would not fail to build - it
# would give every command a FILE that disagrees with the library
# implementing it, and corrupt buffers at run time.  They are the C
# library's interface, and the C library owns them.
#
# These stay in the tree because the native build inside the simulator
# uses them, out of /include, against the compiler that matches them.
#
CCCOWNED = cpm.h errno.h hitech.h setjmp.h stdio.h time.h
SYSHDRS  = $(filter-out $(CCCOWNED),$(notdir $(wildcard include/*.h)))

.PHONY: all update install install-man clean clobber doc sysinstall

#
# First, because make takes the first target in the file as the
# default goal and .PHONY is not one.  Typing "make" here compiles the
# tree with the cross compiler and stops; it does not copy anything
# into the filesystem the simulator boots.  That is install, and it is
# a separate thing to ask for - GNUmakefile.inc orders these the same
# way for the same reason.
#
# The librarian, built for the HOST.
#
# The archives this tree makes are read by a linker that also runs on
# the host, so the program that writes them has to be a host binary -
# and cmd/ar builds the z80 one, which cannot.  Same source, two
# targets.
#
# It is called mxar and not ar because /usr/local/bin/ar would shadow
# the system's for everything on this machine.  The "mx" prefix is
# what this tree uses for a host program that speaks micronix - see
# mxccc in ccc - and NOT the ".mx" suffix, which means a micronix
# executable, which this is not: it is an ELF binary that reads and
# writes micronix archives.
#
# It is built HERE and not in cmd/ar because DIRS is "lib cmd sys",
# and the libraries are archived before cmd is reached.
#
ARMX	 = mxar
ARSRC	 = cmd/ar/ar.c
HOSTCC	?= gcc
HOSTCFLAGS ?= -w -std=gnu89 -Dlinux

all: $(ARMX)
	$(RECURSE)

#
# BELOW "all", not above it.  The first explicit target in a makefile
# is the default goal, and with this rule above, "make" here meant
# "make mxar": it checked one binary, recursed into nothing, and
# exited 0 with the libraries it was asked to rebuild still missing.
# GNUmakefile.inc has the same warning over $(OBJS): $(HSRCS), for
# the same reason, and this is the second time.
#
$(ARMX): $(ARSRC)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(ARSRC)

install:
	$(RECURSE)

#
# The man pages alone, without rebuilding or reinstalling anything -
# what you want after fixing a page.  The rule reaches every leaf
# that has one; the ones that do not install nothing.
#
install-man:
	$(RECURSE)

sysinstall:
	$(SUDO) mkdir -p $(SYSINC)/sys
	$(SUDO) cp include/sys/*.h $(SYSINC)/sys
	(cd include && $(SUDO) cp $(SYSHDRS) $(SYSINC))
	@echo "installed: $(SYSINC)/sys, and $(words $(SYSHDRS)) headers beside it"
	@echo "not installed, ccc owns them: $(CCCOWNED)"

#
# The top of the source tree the simulator sees.  The recursion fills
# in /usr/src/lib, /usr/src/cmd and the rest; this ships the makefile
# that drives them, which nothing did - /usr/src/makefile was a 2023
# copy that still had "cd sys" commented out and had never heard of
# libexec.  Beside it were a Makefile and a GNUmakefile from the same
# era, neither of which anything reads on that machine.
#
# --keep-newer-files, as everywhere else: a makefile edited inside
# micronix is not to be overwritten by an older one from here.
#
ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src
NOWARN	 = --warning=no-ignore-newer

src.tar: makefile
	tar -f src.tar -c makefile

update: src.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f src.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f src.tar

doc:
	$(RECURSE)
