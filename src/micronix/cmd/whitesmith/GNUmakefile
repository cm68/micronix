#
# Makefile for cross
#
# cmd/whitesmith/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
CLASS = cmd/
DIR = whitesmith
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

DIRS = asm p1 p2 pp util link lib include

update:
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c Makefile
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)
	for i in $(DIRS); do \
		(cd $$i ; make update) ; \
	done

doc:
	for i in $(DIRS); do \
		(cd $$i ; make doc) ; \
	done

clean:
	for i in $(DIRS); do \
		(cd $$i ; make clean) ; \
	done

clobber: clean
	for i in $(DIRS); do \
		(cd $$i ; make clobber) ; \
	done
	rm -f $(DIR).tar
