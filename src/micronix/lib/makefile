#
# rebuild the micronix native library
#
# micronix/lib/Makefile
#
# Changed: <2023-07-10 14:32:51 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd libc ; make
	cd libutil ; make

install:
	cd libc ; make install
	cd libutil ; make install

upgrade:
	cd libc ; make upgrade
	cd libutil ; make upgrade
