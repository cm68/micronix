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
# The top GNUmakefile is a leaf like the rest: DEPTH is the distance to
# GNUmakefile.inc, which sits here, so it is ".".  Everything shared
# comes from there - the compiler and librarian, the install/clean/
# update rules, and install-hwsim/install-usersim for the simulators.
# What stays below is only what is top-specific: the host tools built
# here rather than in a DIR, and the header install that needs sudo.
#
DEPTH = .

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

include $(DEPTH)/GNUmakefile.inc

.PHONY: hostcc hostrt sysinstall

#
# The librarian, built for the HOST.
#
# The archives this tree makes are read by a linker that also runs on
# the host, so the program that writes them has to be a host binary -
# and cmd/ar builds the z80 one, which cannot.  Same source, two
# targets.
#
# It is called mxar and not ar because the system's ar would shadow
# the one that reads micronix archives for everything on this machine.
# The "mx" prefix is what this tree uses for a host program that speaks
# micronix - see mxccc and mxnm - and NOT the ".mx" suffix, which means
# a micronix executable, which this is not: it is an ELF binary that
# reads and writes micronix archives.
#
# It is built HERE and not in cmd/ar because DIRS is "include lib
# libexec cmd sys", and the libraries are archived before cmd is
# reached.  The symbol lister is the same shape: cmd/nm builds the z80
# one, and this builds mxnm beside mxar, in $(BIN).
#
ARMX	 = $(BIN)/mxar
ARSRC	 = cmd/ar/ar.c
NMX	 = $(BIN)/mxnm
NMSRC	 = cmd/nm/nm.c cmd/nm/wsobj.c
SETDEVX	 = $(BIN)/setdev
SETDEVSRC = cmd/setdev/setdev.c
HOSTCC	?= gcc
HOSTCFLAGS ?= -w -std=gnu89 -Dlinux

#
# The host tools are prerequisites of all, not part of the recursion,
# so the librarian exists before lib is archived.  This is a
# prerequisite line only - the recipe is GNUmakefile.inc's.
#
all: $(ARMX) $(NMX) $(SETDEVX)

#
# clobber removes the host tools and src.tar as well as what the
# recursion removes.  GNUmakefile.inc's clobber reads GENERATED.
#
GENERATED = src.tar $(ARMX) $(NMX) $(SETDEVX)

$(ARMX): $(ARSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(ARSRC)

#
# The host symbol lister.  nm on micronix, mx-prefixed here like mxar
# and mxccc: it reads micronix objects, which the system nm cannot.
# Built from the same sources as cmd/nm, with the machine's own cc.
#
$(NMX): $(NMSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(NMSRC)

$(SETDEVX): $(SETDEVSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(SETDEVSRC)

#
# The cross compiler, built here from these sources.
#
# Before the fork the tree compiled with /usr/local/bin/ccc, which is
# installed out of ~/src/ccc - so "this tree's compiler" was the other
# tree's.  These sources are ours now and this builds them: the seven
# passes and the driver, with the machine's own cc, into $(BIN) and
# $(HOSTROOT)/libexec.
#
# Then the runtime beside it, because the driver adds crt0.o and
# libc.a from its own lib directory whatever else is on the link line,
# and works that directory out from argv[0] - $(BIN)/mxccc reads
# $(HOSTROOT)/lib and $(HOSTROOT)/usr/include.  The archives have to
# exist before anything links, which is why this is a target you ask
# for and not part of all: the libraries are built by the compiler it
# installs.
#
#	make hostcc		build the compiler
#	make			build the tree with it, once lib is made
#	make hostrt		put the runtime where it can find it
#
HOSTCCDIRS = libexec/cpp libexec/c0 libexec/c1 libexec/peep \
	     cmd/asz cmd/ld libexec/astpp cmd/ccc

hostcc: $(ARMX)
	@for d in $(HOSTCCDIRS); do $(MAKE) -C $$d host || exit 1; done
	@echo "host compiler in $(BIN)/mxccc"

hostrt:
	@mkdir -p $(HOSTROOT)/lib/include/sys $(HOSTROOT)/usr/include/sys
	#
	# lib/include is where -m cpm looks: CP/M has no /usr, so the
	# driver points its include path at the compiler's own lib
	# directory and the C library's headers are all there is.
	#
	cp lib/include/*.h $(HOSTROOT)/lib/include
	cp lib/include/sys/*.h $(HOSTROOT)/lib/include/sys
	cp lib/libc/libc.a lib/libu/libu.a lib/libu/crt0.o \
	   lib/libccc/libccc.a $(HOSTROOT)/lib
	#
	# and the CP/M runtime, which -m cpm links instead of libu.a
	# and starts with crtcpm.o instead of crt0.o.
	#
	cp lib/libcpm/libcpm.a lib/libcpm/crtcpm.o $(HOSTROOT)/lib
	#
	# The system headers first and the C library's over them.  Six
	# names are in both sets - cpm.h errno.h hitech.h setjmp.h
	# stdio.h time.h - and the library's are the ones that count:
	# include/GNUmakefile calls them CCCOWNED and installs its own
	# copies of everything BUT those.  The tree's time.h is the v6
	# one, struct time in UINTs, and c0 will not read it.
	#
	cp include/*.h $(HOSTROOT)/usr/include
	cp include/sys/*.h $(HOSTROOT)/usr/include/sys
	cp lib/include/*.h $(HOSTROOT)/usr/include
	cp lib/include/sys/*.h $(HOSTROOT)/usr/include/sys
	@echo "runtime in $(HOSTROOT)/lib"

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
# This update is top-specific: it ships src.tar (the makefile above),
# which GNUmakefile.inc's update does not know about, so it overrides
# that rule.
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
