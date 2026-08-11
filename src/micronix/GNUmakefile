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

.PHONY: all update clean clobber doc

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
