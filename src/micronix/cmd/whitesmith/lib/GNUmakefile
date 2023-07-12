#
# Makefile for cross
#
# cmd/whitesmith/lib/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
CLASS = cmd/whitesmith
DIR = lib
ROOT = ~/src/micronix/filesystem
SRCDIR = /usr/src/$(CLASS)/$(DIR)
DEST = $(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

DIRS = libws libm libu

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
