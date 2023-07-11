#
# Makefile for updating the source tree
#
# cmd/whitesmith/pp/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = pp
CLASS = cmd/whitesmith
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

SRCS = *.c *.h

update: 
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c Makefile $(SRCS)
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)

doc: pp.0 p0lex.3 p0main.3 p0subs.3

%.0 : %.mp
	nroff -M.. $< -mws > $@

%.3 : %.ic
	nroff -M.. $< -mws > $@

clean:

clobber: clean
	rm -f *.3 *.0 $(DIR).tar
