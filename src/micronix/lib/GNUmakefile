#
# cross build for the libraries
#
# lib/GNUmakefile
#
# libu was missing from DIRS, so the tree build never produced libu.a
# and it only ever got built by hand in its own directory.  The order
# matters no more than it does for any archive, but libu is the system
# call layer libc sits on, so it goes first.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = libu libc libutil

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
