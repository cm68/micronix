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
# pass0, c0 and c1 are named here.  They want _fmtlong and _fmtstr out
# of lib/libccc, which each of their makefiles names in LIBS, and which
# installs as /lib/libccc.a.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd pass0 ; make all
	cd c0 ; make all
	cd c1 ; make all
	cd peep ; make all
	cd astpp ; make all

install:
	cd pass0 ; make install
	cd c0 ; make install
	cd c1 ; make install
	cd peep ; make install
	cd astpp ; make install

clean:
	cd pass0 ; make clean
	cd c0 ; make clean
	cd c1 ; make clean
	cd peep ; make clean
	cd astpp ; make clean

clobber:
	cd pass0 ; make clobber
	cd c0 ; make clobber
	cd c1 ; make clobber
	cd peep ; make clobber
	cd astpp ; make clobber
