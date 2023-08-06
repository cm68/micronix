#
# Makefile for updating source tree
#
# cmd/GNUmakefile
#
# Changed: <2023-07-27 12:00:39 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = ar cc cp echo ld ls Make mkdir mknod pp pwd rm s vi

all:
	for i in $(DIRS) ; do (cd $$i ; make all) ; done
	
clean:
	for i in $(DIRS) ; do (cd $$i ; make clean) ; done

clobber: clean
	for i in $(DIRS) ; do (cd $$i ; make clobber) ; done

cross:
	for i in $(DIRS) ; do (cd $$i ; make cross) ; done
