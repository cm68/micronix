#
# Makefile for building generic, non-compiler specific library
#
# lib/GNUmakefile
#
# Changed: <2023-09-04 09:09:09 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = libc libutil

all:
	for i in $(DIRS) ; do (cd $$i ; make all) ; done
	
clean:
	for i in $(DIRS) ; do (cd $$i ; make clean) ; done

clobber: clean
	for i in $(DIRS) ; do (cd $$i ; make clobber) ; done

update:
	for i in $(DIRS) ; do (cd $$i ; make update) ; done
