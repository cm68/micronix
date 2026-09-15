#
# cross build for the standalone programs
#
# stand/GNUmakefile
#
# boot is the two-level hard disk loader, cross built with ccc, and the
# only thing here the cross build touches.  formatmw is a CP/M program
# that cannot become a micronix one - its own makefile says so - so it
# is not in DIRS, exactly as the native makefile beside this has it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = boot

RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

#
# This directory has a native makefile of its own and the recursion
# alone would leave /usr/src/stand holding the subdirectories and
# nothing to drive them.  So update ships it, the way lib and cmd do,
# and --keep-newer-files for the same reason: a makefile edited inside
# micronix is not to be overwritten by one older than it.
#
ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/stand
NOWARN	 = --warning=no-ignore-newer

.PHONY: all update install install-man clean clobber doc

all:
	$(RECURSE)

stand.tar: Makefile
	tar -f stand.tar -c Makefile

update: stand.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f stand.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	$(RECURSE)

#
# Nothing of its own here; this exists so that the rule reaches the
# directories below.
#
install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f stand.tar

doc:
	$(RECURSE)
