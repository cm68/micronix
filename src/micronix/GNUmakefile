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

DIRS = lib cmd sys

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

.PHONY: all update clean clobber doc sysinstall

sysinstall:
	$(SUDO) mkdir -p $(SYSINC)/sys
	$(SUDO) cp include/sys/*.h $(SYSINC)/sys
	(cd include && $(SUDO) cp $(SYSHDRS) $(SYSINC))
	@echo "installed: $(SYSINC)/sys, and $(words $(SYSHDRS)) headers beside it"
	@echo "not installed, ccc owns them: $(CCCOWNED)"

all:
	$(RECURSE)

update:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)

doc:
	$(RECURSE)
