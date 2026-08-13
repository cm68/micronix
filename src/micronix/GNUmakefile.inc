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
LIB	?= /usr/local/bin/wslib
NM	?= /usr/local/bin/wsnm
CCFLAGS	?= -m micronix

# -i and -I are not the same list, and the difference is the whole
# reason the commands would not compile.  ccc resolves <sys/...> out
# of its system include directory, which defaults to a place beside
# the compiler holding a CP/M stat.h and nothing else, and that lookup
# happens BEFORE -I - so no amount of reordering -I could reach the
# tree's sys/stat.h.  Every command that stats a file got the CP/M
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
# against what this build just produced.  cmd/Make is where that shows
# up: system() is in lib/libc and not in the installed libc.a, so Make
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

.PHONY: all update install clean clobber doc disas

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
	$(LIB) -cr $(TARGET) $(OBJS)
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
# Put the built binary into the filesystem the simulator boots.  A
# directory that only recurses, or one whose target is a library, has
# nothing to install: libc.a is linked into the commands here and does
# not exist on the running system.
#
ifeq ($(strip $(TARGET)),)
install:
	$(RECURSE)
else
ifeq ($(suffix $(TARGET)),.a)
install:
	$(RECURSE)
else
install: $(TARGET)
	$(RECURSE)
	mkdir -p $(INSTDIR)
	cp $(TARGET) $(INSTDIR)/$(TARGET)
endif
endif

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
