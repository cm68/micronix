#
# makefile for libexec
#
# micronix/libexec/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, and drives the makefile in each pass's directory.  make
# install is what puts them in /libexec.
#
# This make has no wildcard and no macros to speak of, so each
# directory is named twice per rule, as lib/makefile does.
#
# cpp, c0 and c1 are named here and do not link yet: they want
# _fmtlong and _fmtstr, which live in ccc's ccclib, and there is no
# lib/libccc in this tree to link against.  They are listed because
# the day that arrives they should build, and a directory left out of
# a makefile is a directory nobody notices - which is how cmd/size
# came to have no wsobj.h.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd cpp ; make all
	cd c0 ; make all
	cd c1 ; make all
	cd peep ; make all
	cd astpp ; make all

install:
	cd cpp ; make install
	cd c0 ; make install
	cd c1 ; make install
	cd peep ; make install
	cd astpp ; make install

clean:
	cd cpp ; make clean
	cd c0 ; make clean
	cd c1 ; make clean
	cd peep ; make clean
	cd astpp ; make clean

clobber:
	cd cpp ; make clobber
	cd c0 ; make clobber
	cd c1 ; make clobber
	cd peep ; make clobber
	cd astpp ; make clobber
