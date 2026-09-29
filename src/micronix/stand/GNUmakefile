#
# cross build for the standalone programs
#
# stand/GNUmakefile
#
# boot is the two-level hard disk loader, cross built with ccc, and the
# only thing here the cross build touches.  formatmw is the CP/M
# original of Morrow's hard disk formatter, and a CP/M program it stays:
# it hands the controller physical dma addresses, writes the channel
# address to physical 0x50 and issues out instructions, and none of that
# survives the MPZ80's map.  So it runs under CP/M on the d1 simulation -
# which is where its sector id table can be read off the wire instead of
# guessed at - and it is not in DIRS for that reason.  It does not need
# to be: what it does inside micronix is cmd/mwformat, which is built and
# installed like any other command, and djformat beside it.
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
