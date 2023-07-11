#
# makefile for post-upgrade builds
# /makefile
#
# Changed: <2023-07-04 10:23:43 curt>
#
# if both Makefile and makefile are present, the new make uses this
# file, while the old make uses Makefile.
#
# so, if you have both Makefile and makefile, it depends on which
# make you are running.
#
# this mechanism will be used to bootstrap the new make on a system where
# the new make does not exist yet.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd include ; make install
	cd lib ; make install
	cd cmd ; make install
	# cd sys ; make install

clean:
	cd include ; make clean
	cd lib ; make clean
	cd cmd ; make clean
	# cd sys ; make clean

clobber: clean
	cd include ; make clobber
	cd lib ; make clobber
	cd cmd ; make clobber
	# cd sys ; make clobber

#
# this rule populates /old with the stock micronix binaries
# and installs the new programs in their place
#
upgrade:
	cd include ; make upgrade
	cd lib ; make upgrade
	cd cmd ; make upgrade

