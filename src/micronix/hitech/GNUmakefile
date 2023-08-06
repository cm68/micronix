#
# cross build makefile for hitech toolchain
#
# micronix/hitech/GNUmakefile
#
# Changed: <2023-06-16 00:42:02 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

BROKEN = optim
DIRS = cpp cref zas

all:
	for i in $(DIRS); do \
		( cd $$i ; $(MAKE) $(MFLAGS) ) ; \
	done

clean:
	for i in $(DIRS); do \
		( cd $$i ; $(MAKE) clean ) ; \
	done

clobber: clean
	for i in $(DIRS); do \
		( cd $$i ; $(MAKE) clobber ) ; \
	done
