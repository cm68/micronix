#
# makefile for cmd
#
# micronix/cmd/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, and drives the makefile in each command's directory.
# make install is what populates the running system - the binary into
# /bin, and the man page into /usr/man/man1 - out of the sources the
# cross build's make update left in /usr/src.
#
# The directories here are the ones whose makefiles have been revved
# onto ccc.  cc is left out because it still builds itself with the
# Whitesmith compiler and its -lS, and far, msh and the rest were
# already out before that.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd make ; make all
	cd ar ; make all
	cd cp ; make all
	cd echo ; make all
	cd form ; make all
	cd ls ; make all
	cd mkdir ; make all
	cd mknod ; make all
	cd pwd ; make all
	cd rm ; make all
	cd s ; make all
	cd sh ; make all
	cd vi ; make all

#
# reset is a script rather than a program, and it is copied with the
# cp just built here rather than with whatever /bin/cp is, which is
# the point of doing it after cp installs itself.
#
install:
	cd make ; make install
	cd ar ; make install
	cd cp ; make install
	cp/cp reset /bin/reset
	cd echo ; make install
	cd form ; make install
	cd ls ; make install
	cd mkdir ; make install
	cd mknod ; make install
	cd pwd ; make install
	cd rm ; make install
	cd s ; make install
	cd sh ; make install
	cd vi ; make install

clean:
	cd make ; make clean
	cd ar ; make clean
	cd cp ; make clean
	cd echo ; make clean
	cd form ; make clean
	cd ls ; make clean
	cd mkdir ; make clean
	cd mknod ; make clean
	cd pwd ; make clean
	cd rm ; make clean
	cd s ; make clean
	cd sh ; make clean
	cd vi ; make clean

clobber:
	cd make ; make clobber
	cd ar ; make clobber
	cd cp ; make clobber
	cd echo ; make clobber
	cd form ; make clobber
	cd ls ; make clobber
	cd mkdir ; make clobber
	cd mknod ; make clobber
	cd pwd ; make clobber
	cd rm ; make clobber
	cd s ; make clobber
	cd sh ; make clobber
	cd vi ; make clobber
