#
# Makefile for updating the source tree
#
# cmd/whitesmith/include/c/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = c
CLASS = cmd/whitesmith/include
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

SRCS = $(wildcard *.h)

update: 
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c $(SRCS)
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)

doc: int012.3 int01.3 int0.3 int12.3 int1.3 int2.3

%.3 : %.ic
	nroff -M.. $< -mws > $@

clean:

clobber: clean
	rm -f *.3 *.0 $(DIR).tar
