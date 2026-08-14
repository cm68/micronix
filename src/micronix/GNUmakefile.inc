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
# the installed toolchain
#
CCC	?= /usr/local/bin/ccc
#
# The librarian is ar, built for the host as $(DEPTH)/mxar - see the
# top level GNUmakefile.  It was wslib, which wrote the whitesmiths
# archive format; ar writes the v7 one, which is what ar(1) has always
# written and what ld now reads as well.
#
LIB	?= $(DEPTH)/mxar
NM	?= /usr/local/bin/wsnm
CCFLAGS	?= -m micronix

# -i and -I are not the same list, and the difference is the whole
# reason the commands would not compile.  ccc resolves <sys/...> out
# of its system include directory, which defaults to a place beside
# the compiler holding a CP/M stat.h and nothing else, and that lookup
# happens BEFORE -I - so no amount of reordering -I could reach the
# tree's sys/stat.h.
#
# WHERE THAT DIRECTORY IS: libdir/../usr/include, and libdir is
# argv[0]/../lib - so a compiler run as desthost/bin/ccc reads
# desthost/usr/include, and one run as /usr/local/bin/ccc reads
# /usr/local/usr/include.  NOT the lib/include beside it, which also
# holds a full set of headers and is the obvious place to look and to
# fix.  Both exist, only one is read, and they can differ for as long
# as nobody notices.
#
# They did differ, for an afternoon.  A change to stdio.h was made in
# ccc's src/include and copied into every installed copy that could be
# found by eye - lib/include, /usr/local/lib/include, the simulated
# filesystem - but not into usr/include, and ccc's own "make install"
# was failing before it reached the headers, so nothing refreshed it.
# Every object built in that time was compiled against the old
# definition while the header on screen said otherwise.  "ccc -n"
# prints the -i it will use and is the way to settle it.  Every command that stats a file got the CP/M
# struct, which has st_mode and st_size and no st_uid, st_gid, st_ino
# or st_mtime, and c0 reported them one at a time as missing members
# of a structure that was never the right one.
#
# -i points the system include search at this tree, which is what the
# native micronix build has always done - sys/Makefile says
# "CFLAGS = -i $(INC)/" and has since it was written.
#
# -I still puts ccc's headers ahead of the tree's, so the five that
# exist in both - cpm.h hitech.h setjmp.h stdio.h time.h - come from
# the compiler that has to agree with them.  The tree's stdio.h is the
# old Whitesmith one and shares almost nothing with ccc's; a command
# linked against the libc built here has to use ccc's.
#
# NOT -i$(DEPTH)/include, tempting as it is.  -i is searched before -I
# and falls through to it, so that one flag would fix sys/stat.h - but
# it also puts the tree's stdio.h, time.h and setjmp.h ahead of ccc's,
# and the tree's stdio.h is the old Whitesmith one with a different
# FILE.  A command compiled against that and linked against the libc
# built here would not fail to build, it would corrupt its own buffers
# at run time.  The five colliding headers are shipped into the
# simulated filesystem's /include for the native compiler, so they
# cannot simply be retired either.
#
# What that leaves shadowed is sys/stat.h: ccc's system directory has
# a CP/M one, with st_mode and st_size and no st_uid, st_gid, st_ino or
# st_mtime, and no S_IFDIR or S_IFMT.  Every command that stats a file
# gets it, and c0 reports the fields one at a time as missing members
# of a structure that was never the right one.  That is ar, cp, ls,
# mkdir, pwd, rm and vi - seven of the eight commands still failing.
#
INC	?= -I/usr/local/lib/include -I$(DEPTH)/include

#
# where the simulated filesystem lives, and where in it these sources
# belong
#
#
# The libraries this tree builds, named ahead of the ones ccc links by
# itself.  ccc appends /usr/local/lib/libc.a and libu.a to the link by
# absolute path, so -L cannot redirect it and a command would other-
# wise be linked against whatever was last installed rather than
# against what this build just produced.  attic/Make is where that
# showed up: system() is in lib/libc and not in the installed libc.a, so Make
# failed to link with "undefined symbol: _system" while the source of
# system() sat two directories away, freshly compiled.  wsld resolves
# in order, so listing them first is enough.
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
SRCS	?= $(CSRCS) $(HSRCS) $(ASSRCS) $(MANSRCS)

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

.PHONY: all update install install-man clean clobber doc disas

#
# cross build
#
%.o : %.c
	$(CCC) $(CCFLAGS) $(INC) -c $<


%.o : %.s
	$(CCC) $(CCFLAGS) -c $<

ifneq ($(strip $(TARGET)),)
ifeq ($(suffix $(TARGET)),.a)
$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(LIB) cr $(TARGET) $(OBJS)
else
# TREELIBS is not a prerequisite: lib is built before cmd from the top,
# and making it one would leave a leaf built on its own reporting "no
# rule to make target ../../lib/libc/libc.a" instead of building.
$(TARGET): $(OBJS)
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

clean:
	$(RECURSE)
	rm -f $(OBJS) $(DOCFILES)

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
$(DIR).tar: $(SRCS) $(MKFILE)
	tar -f $(DIR).tar --ignore-failed-read -c $(MKFILE) $(SRCS)

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
