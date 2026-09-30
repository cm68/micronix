#
# Makefile include file for the micronix cross build
#
# GNUmakefile.inc
#
# THE CROSS BUILD.  Every GNUmakefile in this tree includes this file
# and builds with the compiler installed in /usr/local, in the tree.
# The "makefile" beside each GNUmakefile is the native build that runs
# inside the simulator; it is the check, never the truth, and nothing
# here knows about it beyond shipping it out with the sources.
#
# Nothing below reaches into a compiler build tree.  If a tool is
# missing from /usr/local, ccc's install rule is what needs fixing.
#
# The four rules every makefile in this tree answers to:
#	all		cross builds with ccc
#	update		copies the sources into the simulated filesystem
#	clean		removes intermediates
#	clobber		removes targets
#
# A calling makefile defines:
#	DEPTH	levels up to this file, in the form ../..
#	DIR	this directory's name
#	CLASS	the path prefix above DIR inside /usr/src, if any
#	DIRS	subdirectories to recurse into, if any
#	TARGET	what to build; defaults to $(DIR), and a .a is archived.
#		Set it empty in a directory that only recurses.
#	SRCS	overrides the default wildcard source lists
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables
.SUFFIXES:

#
# THE COMPILER IS THIS TREE'S OWN.
#
# desthost/bin/mxccc is built from the sources in libexec and cmd/ccc
# by "make hostcc" at the top of this tree.  It has to exist before
# anything compiles, and the runtime it links against has to exist
# before anything links, so a tree with nothing built in it goes:
#
#	make hostcc		the passes and the driver, with cc
#	make -C lib		libc, libu, libccc, with mxccc
#	make hostrt		the archives where mxccc looks for them
#	make			everything else
#
# Override CCC on the command line to build with something else.
#
HOSTROOT	?= $(DEPTH)/../..
BIN		?= $(HOSTROOT)/bin
CCC		?= $(BIN)/mxccc
#
# The librarian is ar, built for the host as $(BIN)/mxar - see the top
# level GNUmakefile.  It was wslib, which wrote the whitesmiths archive
# format; ar writes the v7 one, which is what ar(1) has always written
# and what ld now reads as well.  The symbol lister is nm on micronix
# and mxnm on the host - mx-prefixed like mxccc and mxar, and not the
# obsolete Whitesmiths name wsnm.
#
LIB	?= $(BIN)/mxar
NM	?= $(BIN)/mxnm
#
# -O IS THE PEEPHOLE PASS, and on this machine it is not a nicety.  The
# heap is what is left of 64k after the image, so a byte of text is a
# byte the program cannot allocate - and c0 was failing to compile
# declare.c, the largest source in the compiler, by less than a
# kilobyte.  -O takes 829 bytes
# off c0 and that is the difference between it building the compiler
# and not.
#
# peep is in libexec and is built by this tree.
#
CCFLAGS	?= -m micronix -O

# -i and -I are not the same list.
#
# ccc resolves <sys/...> out of its system include directory, and that
# lookup happens BEFORE -I - so no amount of reordering -I can reach a
# header the system directory shadows.  Left to itself the system
# directory is libdir + INCSUF: bin/../ccc/include, a second copy of
# these headers that something has to keep in step with the tree, and
# a silent disagreement the day it is not.  That is not a worry to
# carry, it is a bug we have already had: mkfs and the kernel were
# built against two vintages of sys/fs.h and wrote s_time to two
# different offsets, and the guest's clock came up in the wrong year
# because of it.  So INC names the system directory itself - the
# tree's own lib/include - and there is no copy left to go stale.  A
# host tool compiling with -I$(DEPTH)/micronix/include now reads the
# same files this does.
#
# Which set goes in -i is not a choice.  The system directory is
# searched first, so the set that has to win the six names below has
# to be the one in -i, and that is the C library's.
#
#
# THE HEADERS ARE THIS TREE'S OWN, both sets.
#
# lib/include is the C library's - stdio.h, ctype.h, fcntl.h, the ones
# a program includes.  include is the system's - the v6 headers, struct
# time and the rest.  The two are different sets and both are wanted:
# /usr/include on micronix holds them merged.
#
# Six names are in both - cpm.h errno.h hitech.h setjmp.h stdio.h
# time.h - and the library's are the ones that count: include/GNUmakefile
# calls them CCCOWNED and installs its own copies of everything BUT
# those.  The tree's stdio.h is the old Whitesmith one with a different
# FILE, so a command linked against the libc built here has to use the
# library's; the tree's time.h is the v6 one, struct time in UINTs, and
# c0 will not read it.
#
INC	?= -i$(DEPTH)/lib/include -I$(DEPTH)/include

#
# where the simulated filesystem lives, and where in it these sources
# belong
#
#
# The libraries this tree builds, named ahead of the runtime ccc appends
# to the link by absolute path.  -L cannot redirect that, so without
# these a command would be linked against whatever was last installed
# rather than against what this build just produced.
#
# ld resolves the archives by fixpoint, not in order: it rescans until a
# pass adds nothing, so naming them here is enough and the order does
# not matter.
#
TREELIBS ?= $(DEPTH)/lib/libc/libc.a $(DEPTH)/lib/libu/libu.a

ROOT	?= ~/src/micronix/filesystem
ifeq ($(strip $(CLASS)),)
SRCDIR	?= /usr/src/$(DIR)
else
SRCDIR	?= /usr/src/$(CLASS)/$(DIR)
endif
DEST	= $(ROOT)$(SRCDIR)
NOWARN	= --warning=no-ignore-newer

#
# Where the binary goes in the filesystem.  Everything this tree
# builds lives in /bin on the running system - that is where cp, ls,
# make, mkdir, mknod, pwd, rm, sh, vi and form are on the 1.6
# standalone - so /bin is the default and a directory that belongs
# elsewhere says so.  init is the one that does: /etc/init.
#
BINDIR	?= /bin
INSTDIR	 = $(ROOT)$(BINDIR)

#
# Where a library goes.  Not BINDIR with a different name on it: ccc
# looks for libc.a in libdir and libdir is worked out as bin/../lib,
# so this is /lib and a directory does not get to move it.
#
LIBDIR	 = /lib
LIBINSTDIR = $(ROOT)$(LIBDIR)

#
# Where man(1) looks: /usr/man/man<section>, then /usr/help - see
# man/README, which read that out of the /bin/man binary.  A
# directory whose page is not in section 1 sets MANSEC itself.
#
MANSEC	?= 1
MANDIR	?= /usr/man/man$(MANSEC)
MANINSTDIR = $(ROOT)$(MANDIR)

CSRCS	?= $(wildcard *.c)
HSRCS	?= $(wildcard *.h)
ASSRCS	?= $(wildcard *.s)
DOCSRCS	?= $(wildcard *.mp *.ic)
#
# A man page ships with the source it documents, so that the makefile
# beside it has something to install when the native build runs.
#
MANSRCS	?= $(wildcard *.1)
#
# Anything else a directory ships that none of the wildcards above
# catch - a data file, a termcap entry - lists itself here.  It rides
# the tar beside the sources but is not a build input.
#
EXTRA	?=
SRCS	?= $(CSRCS) $(HSRCS) $(ASSRCS) $(MANSRCS) $(EXTRA)

MKFILE	 = $(wildcard ?akefile)
DOCFILES = $(DOCSRCS:.mp=.0) $(DOCSRCS:.ic=.3)
OBJS	?= $(CSRCS:.c=.o) $(ASSRCS:.s=.o)

TARGET	?= $(DIR)

#
# What it is called once installed, which is nearly always what it is
# called here.  cpp is the exception: the driver runs it as pass0, so
# that is the name it goes into /libexec under - which is what ccc's
# own build has always done, "$(INSTALL) cpp $(HOSTLIBEXEC)/pass0".
# The directory, the source and the program stay cpp.
#
INSTNAME ?= $(TARGET)

#
# Recursion, as a canned recipe so that every rule descends the same
# way.  The "&&" is the point: with a bare "cd $$d ; make $@" a
# directory that is named in DIRS but does not exist leaves the
# subshell in the current directory, where it runs the same rule
# again - and again, until make runs out of process slots.  That is
# what "pp" in cmd/DIRS used to do.
#
RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

#
# The man page goes in beside the program it documents.  MANSRCS is
# the wildcard, so a directory with no page of its own installs
# nothing and says nothing about it; the test is here rather than in a
# conditional so that one canned recipe serves both "install" and
# "install-man".
#
INSTALLMAN = @if [ -n "$(strip $(MANSRCS))" ] ; then \
		mkdir -p $(MANINSTDIR) ; \
		cp $(MANSRCS) $(MANINSTDIR) ; \
	fi

#
# DELETE A TARGET WHOSE RECIPE FAILED.  Without this, make leaves what
# the failed command had already written, and a zero-length file is
# still newer than its source - so the next make says nothing needs
# doing and the build carries on around the hole.
#
# lib/libu/sbrk.o is how this was found.  The assembler died partway
# through it, leaving an empty sbrk.o that make then considered up to
# date; ar archived the empty member, and every native link after that
# failed with "undefined symbol: _sbrk" while sbrk.s sat there looking
# perfectly correct, because it was.
#
.DELETE_ON_ERROR:

#
# "make" means "make all".
#
# The default goal is otherwise the first real target below, which is
# $(TARGET) - right in every directory that builds one thing, and
# wrong in the first one that does not.  cmd/diff builds diffh beside
# diff and names it in TARGETS; all knows about it, $(TARGET) does
# not, so a bare make left diffh unbuilt and make install then had
# nothing to install.  The native makefile beside this one puts all
# first and has always meant it.
#
.DEFAULT_GOAL := all

.PHONY: all update install install-man clean clobber doc disas \
	install-hwsim install-usersim

#
# cross build
#
# The assembly rule comes FIRST, and the order is the whole of it.
# make takes the first pattern rule that matches, so with these the
# other way round a name having both a .c and a .s built the .c and
# the assembly beside it was never assembled at all.  libc has six of
# those - atoi, index, rindex, strdup, strncmp and memcpy - five of
# them listed in that directory's NOTBUILT_C as superseded, and none
# of them actually superseding anything.
#
# These are the only pairs in the tree, so this reaches them and
# nothing else.
%.o : %.s
	$(CCC) $(CCFLAGS) -c $<


%.o : %.c
	$(CCC) $(CCFLAGS) $(INC) -c $<

ifneq ($(strip $(TARGET)),)
ifeq ($(suffix $(TARGET)),.a)
$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(LIB) cr $(TARGET) $(OBJS)
else
# TREELIBS is not a prerequisite: lib is built before cmd from the top,
# and making it one would leave a leaf built on its own reporting "no
# rule to make target ../../lib/libc/libc.a" instead of building.
#
# THE LIBRARIES ARE PREREQUISITES TOO.  This read "$(TARGET): $(OBJS)",
# so editing libc and rebuilding relinked nothing: make saw every
# object still newer than its source, said the program was up to date,
# and install copied the old binary out.  A change to the C library
# reached a program only when that program's own source happened to
# change as well.
#
# It cost an afternoon of measuring an allocator that was not in the
# binary being measured - the instrumented malloc.o sat in libc.a
# while c0 went on running the one it was linked with.
#
$(TARGET): $(OBJS) $(LIBS) $(TREELIBS)
	$(CCC) $(CCFLAGS) -o $(TARGET) $(OBJS) $(LIBS) $(TREELIBS)
endif
endif

all: $(TARGET) $(TARGETS)
	$(RECURSE)

#
# EVERY object depends on EVERY header here.
#
# Without this, editing a header rebuilds nothing, and what you run is
# objects compiled against one version of a declaration linked against
# another.  It cost an afternoon: adding a function to sh.h left sh.o
# as it was, and the shell that came out signalled its own process
# group - which looked for a while like a code generator bug, and was
# not.  The native makefiles beside these have always had
# "$(OBJS): $*.c $(HDRS)".
#
# It goes HERE, below "all", and not up with the pattern rules: the
# first explicit target in a makefile is the default goal, and put
# above, this one made "make" mean "make glob.o".
#
# Per-object dependencies would be better and want a compiler that
# will emit them; this is coarse and correct.
#
$(OBJS): $(HSRCS)

#
# Put what was built into the filesystem the simulator boots.  A
# directory that only recurses has nothing to install.
#
# A LIBRARY INSTALLS TOO, and used to not.  The reasoning here was
# that "libc.a is linked into the commands and does not exist on the
# running system", which is true of the cross build and false of the
# machine: a native makefile in cmd says "ccc -o sh sh.o" and names no
# library at all, because the driver adds crt0.o, libc.a and libu.a
# from libdir - bin/../lib, so /lib - all by itself.  Nothing there
# meant nothing on that machine could link.  /lib is where a person
# looks for things to link against and this is what puts them there.
#
ifeq ($(strip $(TARGET)),)
install:
	$(RECURSE)
	$(INSTALLMAN)
else
ifeq ($(suffix $(TARGET)),.a)
install: $(TARGET)
	$(RECURSE)
	mkdir -p $(LIBINSTDIR)
	rm -f $(LIBINSTDIR)/$(TARGET)
	cp $(TARGET) $(LIBINSTDIR)/$(TARGET)
	$(INSTALLMAN)
else
#
# rm BEFORE cp, and chmod AFTER it.  cp opens the destination for
# writing and the destination is a program: /etc/init came off the 1.6
# disk as mode ---x------, execute and nothing else, so the copy
# failed with "Permission denied" on a file this account owns
# outright.  Removing the name first replaces the file instead of
# writing through it.
#
# And that is what turned up the missing execute bit.  Copying ONTO a
# file keeps the mode it had, which is the only reason these ever came
# out executable; copying to a name that is not there any more creates
# it fresh under the umask, 664.  There is no chmod here to paper over
# it - ld sets 0755 on what it writes now, because the thing a linker
# produces is a program - and cp carries that mode to a new file.
#
install: $(TARGET)
	$(RECURSE)
	mkdir -p $(INSTDIR)
	rm -f $(INSTDIR)/$(INSTNAME)
	cp $(TARGET) $(INSTDIR)/$(INSTNAME)
	$(INSTALLMAN)
endif
endif

#
# The pages on their own, without rebuilding or reinstalling anything
# - which is what you want after fixing a page, and what the native
# makefiles have always had.
#
install-man:
	$(RECURSE)
	$(INSTALLMAN)

#
# The simulators' host binaries go into the top level bin/ as copies,
# not symlinks, which break when the tree is moved.  install-usersim
# does that for the userland simulator.  hwsim, though, boots a disk
# image rather than the filesystem directory, so install-hwsim puts the
# built source and binaries ONTO that image instead: it builds a tar of
# the tree install+update assembled in $(ROOT) and imports it with
# mnix tar x.
#
install-usersim:
	$(MAKE) -C $(HOSTROOT)/src/usersim
	mkdir -p $(BIN)
	cp $(HOSTROOT)/src/usersim/sim $(BIN)/

#
# The image hwsim boots, and the tool that writes into it.  Both live
# beside the tree; override on the command line for a scratch copy.
#
# The image is built ab initio rather than copied from a "blessed"
# snapshot: initialize simulates what FORMATMW does, mkfs makes the
# filesystem, and tar x puts the built source and binaries on it.
#
HWIMAGE   ?= $(HOSTROOT)/disks/hdinstall/hddma-0.vol
HWBLESSED ?= $(HOSTROOT)/disks/hdinstall/hddma-0.blessed
HWMEDIA   ?= m16
#
# The kernel is copied from the blessed image, whose root device is the
# 5 meg m5a (3/0).  It is re-patched here for the volume HWMEDIA names:
# m5 is 3/0, m10 is 3/4, m16 is 3/8, m32 is 3/12, m40 is 3/16 - see
# hddma.4.
#
HWROOTDEV ?= 3/8
HWROOTMAJ = $(firstword $(subst /, ,$(HWROOTDEV)))
HWROOTMIN = $(lastword $(subst /, ,$(HWROOTDEV)))
BOOTIMG   ?= $(HOSTROOT)/src/micronix/stand/boot/bootimg
MNIX      ?= $(HOSTROOT)/src/tools/mnix
SETDEV    ?= $(BIN)/setdev

#
# The userland that goes on the image, enumerated by name rather than
# by tarring the whole filesystem directory - which holds the source,
# the man pages, ccc's temporaries and whatever a compile left behind.
# The compiler and its headers (lib, libexec, include, usr/include) are
# deliberately not here: they are the build tree, not the running system.
#
# dev is not tar'd: its entries are device nodes, which a tar archive
# cannot carry.  They are made below with mknod, from the list read out
# of the blessed image.
HWSIM_FILES = bin etc usr \
	finstall hdinit m5init m10init m16init \
	.sh .login

install-hwsim: $(MNIX) $(SETDEV)
	$(MNIX) initialize $(HWMEDIA) $(HWIMAGE)
	$(MNIX) mkfs -i $(BOOTIMG) $(HWIMAGE)
	$(MNIX) -f $(HWBLESSED) read /micronix $(HOSTROOT)/micronix.k
	$(SETDEV) $(HOSTROOT)/micronix.k $(HWROOTDEV) 0/0
	$(MNIX) -f $(HWIMAGE) write $(HOSTROOT)/micronix.k /micronix
	rm -f $(HOSTROOT)/micronix.k
	tar -C $(ROOT) -c -f $(HOSTROOT)/hwsim.tar \
	    --exclude='usr/src' --exclude='usr/man' $(HWSIM_FILES)
	$(MNIX) -f $(HWIMAGE) tar x $(HOSTROOT)/hwsim.tar
	rm -f $(HOSTROOT)/hwsim.tar
	$(MNIX) -f $(HWIMAGE) mkdir /dev
	$(MNIX) -f $(HWBLESSED) devlist > $(HOSTROOT)/devlist
	while read type major minor name; do \
		if [ "$$name" = root ] || [ "$$name" = swap ]; then continue; fi; \
		$(MNIX) -f $(HWIMAGE) mknod /dev/$$name $$type $$major $$minor; \
	done < $(HOSTROOT)/devlist
	rm -f $(HOSTROOT)/devlist
	$(MNIX) -f $(HWIMAGE) mknod /dev/root b $(HWROOTMAJ) $(HWROOTMIN)
	$(MNIX) -f $(HWIMAGE) mknod /dev/swap b $(HWROOTMAJ) $(HWROOTMIN)

clean:
	$(RECURSE)
	rm -f $(OBJS) $(DOCFILES) *.nam *.tok *.x

clobber: clean
	$(RECURSE)
	rm -f $(TARGET) $(TARGETS) $(DIR).tar $(GENERATED)

#
# populate the filesystem source tree.  A directory with no sources of
# its own - one that only recurses - has nothing to ship, so the tar
# is skipped rather than run on an empty file list.
#
ifeq ($(strip $(SRCS)$(MKFILE)),)
update:
	$(RECURSE)
else
#
# -h, and then checked: what ships has to be the file, not the link.
# micronix has no symlinks at all, so tar x would refuse the entry
# outright and the source would simply not arrive.  usersim hides
# this - it resolves an absolute host path quite happily - so the
# check is here, where it fails on the machine doing the building.
#
$(DIR).tar: $(SRCS) $(MKFILE)
	tar -f $(DIR).tar -h --ignore-failed-read -c $(MKFILE) $(SRCS)
	@if tar -tvf $(DIR).tar | grep -q '^l'; then \
		echo "$(DIR).tar ships a symlink; micronix cannot extract one" >&2; \
		tar -tvf $(DIR).tar | grep '^l' >&2; \
		rm -f $(DIR).tar; exit 1; \
	fi

update: $(DIR).tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)
endif

doc: $(DOCFILES)
	$(RECURSE)

%.0 : %.mp
	nroff -M.. $< -mws > $@

%.3 : %.ic
	nroff -M.. $< -mws > $@

disas:
	disas $(DEST)/$(TARGET) >$(TARGET).dis
