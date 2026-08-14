#
# cross build for the system headers
#
# include/GNUmakefile
#
# There was no GNUmakefile here and include was not in the top level's
# DIRS, so the cross build never looked at this directory: /usr/src
# had headers in it that nothing in the tree was still shipping, and
# four of them - setjmp.h, stdio.h, sys/con.h and sys/proc.h - had
# drifted from the ones beside this file.
#
# Two destinations, and they are not the same thing:
#
#	/usr/src/include	the SOURCE, so the native build has it
#	/usr/include		the INSTALLED headers, what cpp reads
#
# ccc resolves <sys/...> out of libdir/../usr/include - see tools/ccc.c
# - and libdir is bin/../lib, so a compiler in /bin reads /usr/include.
# That is why install writes there and not to /include, which holds
# the old Whitesmiths set and is left alone.
#
# NOT INSTALLED: the six in CCCOWNED.  A command links the libc built
# in lib/libc, and that libc is compiled against ccc's stdio.h; this
# tree's is the Whitesmiths one and its FILE is a different shape.
# Installing ours over ccc's would not fail to build - it would give
# every command a FILE that disagrees with the library implementing
# it, and corrupt buffers at run time.  The top level GNUmakefile
# holds them out of $(SYSINC) for the same reason and says so at
# greater length.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = sys

RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/include
INCDIR	 = $(ROOT)/usr/include
NOWARN	 = --warning=no-ignore-newer

CCCOWNED = cpm.h errno.h hitech.h setjmp.h stdio.h time.h
HDRS	 = $(wildcard *.h)
OURS	 = $(filter-out $(CCCOWNED),$(HDRS))

MKFILE	 = $(wildcard ?akefile)

.PHONY: all update install install-man clean clobber doc

#
# Nothing to compile.  A header is what it is.
#
all:
	$(RECURSE)

include.tar: $(HDRS) $(MKFILE)
	tar -f include.tar -c $(MKFILE) $(HDRS)

update: include.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f include.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	$(RECURSE)
	mkdir -p $(INCDIR)
	cp $(OURS) $(INCDIR)
	@echo "installed $(words $(OURS)) headers in /usr/include"
	@echo "not installed, ccc owns them: $(CCCOWNED)"

install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber: clean
	$(RECURSE)
	rm -f include.tar

doc:
	$(RECURSE)
