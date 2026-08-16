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
# The 2.11BSD ports - cat chgrp chmod cmp date dd diff du grep ln mv
# od pr sed tee - were carried here as a NEW comment while they earned
# their place, and are in the rules now.
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
	cd cat ; make all
	cd ccc ; make all
	cd chgrp ; make all
	cd chmod ; make all
	cd cmp ; make all
	cd cp ; make all
	cd date ; make all
	cd dd ; make all
	cd diff ; make all
	cd du ; make all
	cd echo ; make all
	cd form ; make all
	cd grep ; make all
	cd init ; make all
	cd less ; make all
	cd ln ; make all
	cd ls ; make all
	cd man ; make all
	cd mkdir ; make all
	cd mknod ; make all
	cd more ; make all
	cd mv ; make all
	cd nm ; make all
	cd od ; make all
	cd pr ; make all
	cd pwd ; make all
	cd rm ; make all
	cd s ; make all
	cd sed ; make all
	cd sh ; make all
	cd size ; make all
	cd sync ; make all
	cd tar ; make all
	cd tee ; make all
	cd vi ; make all

#
# reset is a script rather than a program, and it is copied with the
# cp just built here rather than with whatever /bin/cp is, which is
# the point of doing it after cp installs itself.
#
install:
	cd make ; make install
	cd ar ; make install
	cd cat ; make install
	cd ccc ; make install
	cd chgrp ; make install
	cd chmod ; make install
	cd cmp ; make install
	cd cp ; make install
	cp/cp reset /bin/reset
	cd date ; make install
	cd dd ; make install
	cd diff ; make install
	cd du ; make install
	cd echo ; make install
	cd form ; make install
	cd grep ; make install
	cd init ; make install
	cd less ; make install
	cd ln ; make install
	cd ls ; make install
	cd man ; make install
	cd mkdir ; make install
	cd mknod ; make install
	cd more ; make install
	cd mv ; make install
	cd nm ; make install
	cd od ; make install
	cd pr ; make install
	cd pwd ; make install
	cd rm ; make install
	cd s ; make install
	cd sed ; make install
	cd sh ; make install
	cd size ; make install
	cd sync ; make install
	cd tar ; make install
	cd tee ; make install
	cd vi ; make install

clean:
	cd make ; make clean
	cd ar ; make clean
	cd cat ; make clean
	cd ccc ; make clean
	cd chgrp ; make clean
	cd chmod ; make clean
	cd cmp ; make clean
	cd cp ; make clean
	cd date ; make clean
	cd dd ; make clean
	cd diff ; make clean
	cd du ; make clean
	cd echo ; make clean
	cd form ; make clean
	cd grep ; make clean
	cd init ; make clean
	cd less ; make clean
	cd ln ; make clean
	cd ls ; make clean
	cd man ; make clean
	cd mkdir ; make clean
	cd mknod ; make clean
	cd more ; make clean
	cd mv ; make clean
	cd nm ; make clean
	cd od ; make clean
	cd pr ; make clean
	cd pwd ; make clean
	cd rm ; make clean
	cd s ; make clean
	cd sed ; make clean
	cd sh ; make clean
	cd size ; make clean
	cd sync ; make clean
	cd tar ; make clean
	cd tee ; make clean
	cd vi ; make clean

clobber:
	cd make ; make clobber
	cd ar ; make clobber
	cd cat ; make clobber
	cd ccc ; make clobber
	cd chgrp ; make clobber
	cd chmod ; make clobber
	cd cmp ; make clobber
	cd cp ; make clobber
	cd date ; make clobber
	cd dd ; make clobber
	cd diff ; make clobber
	cd du ; make clobber
	cd echo ; make clobber
	cd form ; make clobber
	cd grep ; make clobber
	cd init ; make clobber
	cd less ; make clobber
	cd ln ; make clobber
	cd ls ; make clobber
	cd man ; make clobber
	cd mkdir ; make clobber
	cd mknod ; make clobber
	cd more ; make clobber
	cd mv ; make clobber
	cd nm ; make clobber
	cd od ; make clobber
	cd pr ; make clobber
	cd pwd ; make clobber
	cd rm ; make clobber
	cd s ; make clobber
	cd sed ; make clobber
	cd sh ; make clobber
	cd size ; make clobber
	cd sync ; make clobber
	cd tar ; make clobber
	cd tee ; make clobber
	cd vi ; make clobber
