#
# rebuild the micronix native library
#
# micronix/lib/Makefile
#
# Changed: <2023-06-23 00:58:10 curt>

all:
	cd libc ; make
	cd libutil ; make

install:
	cd libc ; make install
	cd libutil ; make install

upgrade:
	cd libc ; make upgrade
	cd libutil ; make upgrade

# vim: tabstop=4 shiftwidth=4 expandtab:

