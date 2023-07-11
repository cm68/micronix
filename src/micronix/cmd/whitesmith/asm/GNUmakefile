#
# Makefile for updating the source tree
#
# cmd/whitesmith/asm/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = asm
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

doc: anat.0 dis80.0 as80.0

%.0 : %.mp
	nroff -M.. $< -mws > $@

clean:

clobber: clean
	rm -f *.0 $(DIR).tar
