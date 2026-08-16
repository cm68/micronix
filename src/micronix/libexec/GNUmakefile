#
# cross build for the passes
#
# libexec/GNUmakefile
#
# WHAT THIS DIRECTORY IS FOR: programs that other programs run.
#
# cmd holds what a person types and libexec holds what ccc execs on
# their behalf - the passes, and the tools the driver reaches for
# without being asked.  They install into /libexec, and the driver
# finds them from its own path as bin/../libexec, so the two move
# together and neither belongs on a search path.
#
# /lib keeps none of them.  That is for things to link against -
# libc.a, crt0.o, the headers - and a person reading it should see
# those and nothing else.
#
# EVERYTHING ccc RUNS IS HERE NOW: cpp, c0, c1, peep, and asz, astpp
# and ld, which were still in cmd.  ld is the one that reads oddly -
# a person does type a linker - but ccc builds its path as
# libexecdir/ld and cannot find one in /bin, and the driver is the
# thing that runs it a thousand times for every once somebody does.
#
# All four build.  The three passes used to link against nothing and
# fail on _fmtlong and _fmtstr; those are in ccc's ccclib and there is
# a lib/libccc now, which each of them names in LIBS.
#
# Their sources were symlinks into ~/src/ccc and the sets had drifted:
# c0 was missing eleven p1*.h, all three wanted astops.h, format.h
# and libutil.h out of ccclib, and c0 and c1 wanted lexeme.h and
# lexops.h out of cpp - pass1 compiles with -I../cpp there.  The
# fork copied all of it in, so what is left really is only the
# library.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = cpp c0 c1 peep asz astpp ld

RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/libexec
NOWARN	 = --warning=no-ignore-newer

.PHONY: all update install install-man clean clobber doc

all:
	$(RECURSE)

#
# update was named in .PHONY and given no rule, so "make update"
# walked in here, matched a phony target with no recipe, did nothing
# and said nothing - and /usr/src had no libexec in it at all.  A
# target named in .PHONY and nowhere else fails exactly this quietly.
#
# It ships this directory's native makefile the way cmd and lib ship
# theirs: the recursion alone would leave /usr/src/libexec holding
# four subdirectories and nothing to drive them.
#
libexec.tar: makefile
	tar -f libexec.tar -c makefile

update: libexec.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f libexec.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	$(RECURSE)

install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f libexec.tar

doc:
	$(RECURSE)
