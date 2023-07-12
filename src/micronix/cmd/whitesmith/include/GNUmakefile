#
# Makefile for updating the source tree
#
# cmd/whitesmith/include/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = include
CLASS = cmd/whitesmith
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

SRCS = $(wildcard *.h)

update: 
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c $(SRCS) Makefile
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)
	cd c; make update

doc:

clean:

clobber: clean
	rm -f $(DIR).tar
