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
# THE LIST IS THE SAME LIST AS DIRS IN THE GNUmakefile, and had come
# apart from it: ccc, init, man, nm, size and sync were built by the
# cross build and not by this one, so the machine could not rebuild
# six of the programs it was running - the compiler driver among them.
# There is no wildcard in this make, so the only thing keeping the two
# together is somebody reading both.
#
# make goes first, and is the one entry whose order matters: it is
# what is running.
#
# ld, asz and astpp used to be here and are in ../libexec now, with
# the passes - see that directory's makefile.
#
# cc is left out because it still builds itself with the Whitesmith
# compiler and its -lS; far, msh and the rest were out before that.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd make ; make all
	cd ar ; make all
	cd ccc ; make all
	cd cp ; make all
	cd echo ; make all
	cd form ; make all
	cd init ; make all
	cd ls ; make all
	cd man ; make all
	cd mkdir ; make all
	cd mknod ; make all
	cd nm ; make all
	cd pwd ; make all
	cd rm ; make all
	cd s ; make all
	cd sh ; make all
	cd size ; make all
	cd sync ; make all
	cd vi ; make all

#
# reset is a script rather than a program, and it is copied with the
# cp just built here rather than with whatever /bin/cp is, which is
# the point of doing it after cp installs itself.
#
install:
	cd make ; make install
	cd ar ; make install
	cd ccc ; make install
	cd cp ; make install
	cp/cp reset /bin/reset
	cd echo ; make install
	cd form ; make install
	cd init ; make install
	cd ls ; make install
	cd man ; make install
	cd mkdir ; make install
	cd mknod ; make install
	cd nm ; make install
	cd pwd ; make install
	cd rm ; make install
	cd s ; make install
	cd sh ; make install
	cd size ; make install
	cd sync ; make install
	cd vi ; make install

clean:
	cd make ; make clean
	cd ar ; make clean
	cd ccc ; make clean
	cd cp ; make clean
	cd echo ; make clean
	cd form ; make clean
	cd init ; make clean
	cd ls ; make clean
	cd man ; make clean
	cd mkdir ; make clean
	cd mknod ; make clean
	cd nm ; make clean
	cd pwd ; make clean
	cd rm ; make clean
	cd s ; make clean
	cd sh ; make clean
	cd size ; make clean
	cd sync ; make clean
	cd vi ; make clean

clobber:
	cd make ; make clobber
	cd ar ; make clobber
	cd ccc ; make clobber
	cd cp ; make clobber
	cd echo ; make clobber
	cd form ; make clobber
	cd init ; make clobber
	cd ls ; make clobber
	cd man ; make clobber
	cd mkdir ; make clobber
	cd mknod ; make clobber
	cd nm ; make clobber
	cd pwd ; make clobber
	cd rm ; make clobber
	cd s ; make clobber
	cd sh ; make clobber
	cd size ; make clobber
	cd sync ; make clobber
	cd vi ; make clobber
