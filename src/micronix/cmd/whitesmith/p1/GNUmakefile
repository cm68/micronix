#
# Makefile for updating the source tree
#
# cmd/whitesmith/p1/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = p1
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

doc: p1.0 p1data.3 p1expr.3 p1func.3 p1gets.3 p1.3 p1main.3 \
	p1puts.3 p1red.3 p1subs.3 p1type.3

%.0 : %.mp
	nroff -M.. $< -mws > $@

%.3 : %.ic
	nroff -M.. $< -mws > $@

clean:

clobber: clean
	rm -f *.0 *.3 $(DIR).tar
