#
# Makefile for updating the source tree
#
# cmd/whitesmith/p2/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = p2
CLASS = cmd/whitesmith
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

SRCS = $(wildcard *.c *.h)

update: 
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c Makefile $(SRCS)
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)

doc: p2.0 p2.3 p2expr.3 p2fmov.3 p2gen.3 \
	p2jump.3 p2main.3 p2subs.3 p2tab.3

%.0 : %.mp
	nroff -M.. $< -mws > $@

%.3 : %.ic
	nroff -M.. $< -mws > $@

clean:

clobber: clean
	rm -f *.0 *.3 $(DIR).tar
