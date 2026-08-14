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
# cpp, c0, c1 and peep are here; asz and astpp are passes too and are
# still in cmd.
#
# All four build.  The three passes used to link against nothing and
# fail on _fmtlong and _fmtstr; those are in ccc's ccclib and there is
# a lib/libccc now, which each of them names in LIBS.
#
# Their sources are symlinks into ~/src/ccc and the sets had drifted:
# c0 was missing eleven p1*.h, all three wanted astops.h, format.h
# and libutil.h out of ccclib, and c0 and c1 wanted lexeme.h and
# lexops.h out of cpp - pass1 compiles with -I../cpp there.  Those
# are linked now, so what is left really is only the library.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = cpp c0 c1 peep

RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/libexec
NOWARN	 = --warning=no-ignore-newer

.PHONY: all update install install-man clean clobber doc

all:
	$(RECURSE)

install:
	$(RECURSE)

install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)

doc:
	$(RECURSE)
