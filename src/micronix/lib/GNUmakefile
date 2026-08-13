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

#
# This directory has a native makefile of its own - the one that runs
# "make install" in each library in turn - and the recursion alone
# would leave /usr/src/lib holding the subdirectories and nothing to
# drive them.  So update ships it, the way cmd does, and with
# --keep-newer-files for the same reason: a makefile edited inside
# micronix is not to be overwritten by an older one from here.
#
ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/lib
NOWARN	 = --warning=no-ignore-newer

.PHONY: all update install clean clobber doc

all:
	$(RECURSE)

lib.tar: makefile
	tar -f lib.tar -c makefile

update: lib.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f lib.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

#
# The cross build does not install these: it links them into the
# commands it builds out of the tree, and putting them on the running
# system is the native install's job - lib/libc/makefile and its
# neighbours copy the archives into /lib.  The rule is here so that a
# tree-wide "make install" walks without stopping.
#
install:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f lib.tar

doc:
	$(RECURSE)
