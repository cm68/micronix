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
# Withholding them is only half the rule.  The other half is that the
# headers that DO belong under those names have to be there, and
# nothing was putting them there: the C library's own headers live in
# lib/include, that directory is not in lib's recursion and has no
# makefile of its own, and /usr/include is written by this rule and by
# nobody else.  So the install fell exactly one set short, and a
# from-scratch native build died on the first file that includes
# stdio.h - "cannot find include file: stdio.h", in libexec/pass0.  The
# seed only ever worked because the 2023 filesystem had the set hand
# copied into it, which is the state the earlier rebuild wiped.
#
#	cp lib/include/*.h          24 headers, ccc's stdio.h among them
#	cp lib/include/sys/*.h      sys/wait.h, which the tree's sys/ has not
#
# The library's go on AFTER ours, the order hostrt uses at the top
# level and for the same reason: the six names are in both sets and
# the library's are the ones that count.  sys/wait.h is not a
# collision - the tree's /usr/include/sys has none - but it is the
# library's, so it goes to /usr/include/sys with the rest of them.
#
# cpm/GNUmakefile does the same two copies, into its own
# /usr/cpm/include, with the CP/M stdio.h over them.
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

#
# The C library's headers, which install beside ours and over them.
# See the top of this file.
#
LIBHDRS	   = $(wildcard ../lib/include/*.h)
LIBSYSHDRS = $(wildcard ../lib/include/sys/*.h)

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
	mkdir -p $(INCDIR)/sys
	cp $(OURS) $(INCDIR)
	cp $(LIBHDRS) $(INCDIR)
	cp $(LIBSYSHDRS) $(INCDIR)/sys
	@echo "installed $(words $(OURS)) system headers in /usr/include"
	@echo "and the C library's $(words $(LIBHDRS)) over them, with sys/wait.h"
	@echo "the tree's own copies of these are not installed: $(CCCOWNED)"

install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber: clean
	$(RECURSE)
	rm -f include.tar

doc:
	$(RECURSE)
