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

# ccc's headers come first, so that the handful which exist in both
# trees - cpm.h hitech.h setjmp.h stdio.h time.h - come from the
# compiler that has to agree with them.  The micronix tree supplies
# the system headers ccc has no equivalent for: types.h, dirent.h,
# sys/dir.h and the rest.
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

CSRCS	?= $(wildcard *.c)
HSRCS	?= $(wildcard *.h)
ASSRCS	?= $(wildcard *.s)
DOCSRCS	?= $(wildcard *.mp *.ic)
SRCS	?= $(CSRCS) $(HSRCS) $(ASSRCS)

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

.PHONY: all update clean clobber doc disas

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
