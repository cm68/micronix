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
# Only peep is here so far.  asz, c0, c1, cpp and astpp are passes
# too and belong beside it; they are still in cmd, where their
# sources are symlinks into ~/src/ccc, and they move when someone
# has a reason to move them rather than for tidiness alone.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = peep

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
