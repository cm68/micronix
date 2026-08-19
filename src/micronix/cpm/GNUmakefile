#
# cross build for CP/M's headers
#
# cpm/GNUmakefile
#
# CP/M's header tree is /usr/cpm/include, a directory of its own, not
# /usr/include - its stdio.h is not Micronix's.  The FILE layer there
# sits over fcb's rather than file descriptors and the flag bits
# differ, so the two cannot share the one header.  Everything else in
# the tree is the C library's own (cpm.h, hitech.h, ctype.h and the
# rest), copied over from lib/include, with the CP/M stdio.h put on
# top of the Micronix one that lives there.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

ROOT	?= ~/src/micronix/filesystem
INCDIR	 = $(ROOT)/usr/cpm/include

.PHONY: all update install install-man clean clobber doc

#
# Nothing to compile.  A header is what it is.
#
all:

update:

install:
	mkdir -p $(INCDIR)
	cp ../lib/include/*.h $(INCDIR)
	cp ../lib/include/sys/*.h $(INCDIR)
	cp include/stdio.h $(INCDIR)
	@echo "installed CP/M headers in /usr/cpm/include"

install-man:

clean:

clobber: clean

doc:
