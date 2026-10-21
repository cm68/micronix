#
# makefile for lib
#
# micronix/lib/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, and drives the makefile in each library's directory.
# make install is what puts the three archives in /lib.
#
# libunix is built first because it is the system call layer everything
# else sits on, and it is in this file at all now - the old one built
# libc and libutil and walked past it.
#
# Nothing here has anything to do with the Whitesmith libraries.  ccc
# returns a value in HL where that compiler returns it in BC, so
# /lib/libS.a cannot be linked against anything built here, and
# patching objects into it - which is what this directory used to do -
# would only produce an archive that is wrong in a new way.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd libunix ; make all
	cd libc ; make all
	cd libutil ; make all
	cd libccc ; make all

install:
	cd libunix ; make install
	cd libc ; make install
	cd libutil ; make install
	cd libccc ; make install

clean:
	cd libunix ; make clean
	cd libc ; make clean
	cd libutil ; make clean
	cd libccc ; make clean

clobber:
	cd libunix ; make clobber
	cd libc ; make clobber
	cd libutil ; make clobber
	cd libccc ; make clobber

#
# verify - the inverse of install, one level down: every
# directory compares its just-built artifact with the one the
# first generation installed.
#
verify:
	cd libunix ; make verify
	cd libc ; make verify
	cd libutil ; make verify
	cd libccc ; make verify
