#
# the native build: in the simulator, and on the hardware
#
# micronix/makefile
#
# There are two makefiles here and only two.  This one is read by the
# make that runs on micronix; GNUmakefile beside it is the cross build
# and is read on the host.  Nothing has to look at which make it is
# running any more.
#
# There used to be a third - a Makefile, capital M, which the OLD make
# preferred and which existed to bootstrap the new one by building
# cmd/Make first.  That is retired: the cross compiler works, so a
# system without a make gets one built for it on the host rather than
# having to climb out of the hole by itself.  Make is in ../attic with
# the other two that lost the name.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# lib, sys and cmd, the same three the GNUmakefile beside this one
# descends into.  sys was commented out here and is not any more.
#
all:
	cd include ; make install
	cd lib ; make install
	cd cmd ; make install
	cd sys ; make install

clean:
	cd include ; make clean
	cd lib ; make clean
	cd cmd ; make clean
	cd sys ; make clean

clobber: clean
	cd include ; make clobber
	cd lib ; make clobber
	cd cmd ; make clobber
	cd sys ; make clobber

#
# this rule populates /old with the stock micronix binaries
# and installs the new programs in their place
#
upgrade:
	cd include ; make upgrade
	cd lib ; make upgrade
	cd cmd ; make upgrade

