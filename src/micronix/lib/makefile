#
# makefile for lib
#
# micronix/lib/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, and drives the makefile in each library's directory.
# make install is what puts the three archives in /lib.
#
# libu is built first because it is the system call layer everything
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
	cd libu ; make all
	cd libc ; make all
	cd libutil ; make all

install:
	cd libu ; make install
	cd libc ; make install
	cd libutil ; make install

clean:
	cd libu ; make clean
	cd libc ; make clean
	cd libutil ; make clean

clobber:
	cd libu ; make clobber
	cd libc ; make clobber
	cd libutil ; make clobber
