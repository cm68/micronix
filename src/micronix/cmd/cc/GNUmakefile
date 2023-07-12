#
# Makefile for updating the source tree
#
# cmd/cc/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIR = cc
CLASS = cmd
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

SRCS = $(wildcard *.c *.h)
CFILES = cc.c

update: 
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c Makefile $(SRCS)
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)

doc:

%.0 : %.mp
	nroff -M.. $< -mws > $@

%.3 : %.ic
	nroff -M.. $< -mws > $@

clean:
	rm -f *.o

clobber: clean
	rm -f *.3 *.0 $(DIR).tar cc

cross:
	cc -fdiagnostics-color=always -g -o cc $(CFILES) 2>&1
