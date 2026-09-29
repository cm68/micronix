#
# micronix native standalone
#
# micronix/stand/Makefile
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

#
# Two things live here and they are not built the same way.  boot is
# the two level hard disk loader, cross built with ccc; formatmw is the
# CP/M original of Morrow's hard disk formatter, and it stays a CP/M
# program - see its own makefile.  What it does inside micronix is
# cmd/mwformat, with cmd/djformat beside it.  formatmw is not in all,
# because c1 still dies on it.
#
DIRS = boot

all install clean clobber:
	@for d in $(DIRS); do \
		( cd $$d && $(MAKE) $@ ) || exit 1; \
	done

formatmw:
	( cd formatmw && $(MAKE) )
