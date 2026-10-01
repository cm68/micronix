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
#
# make goes first, and is the one entry whose order matters: it is
# what is running.
#
# The 2.11BSD ports - cat chgrp chmod cmp date dd diff du grep ln mv
# od pr sed tee - were carried here as a NEW comment while they earned
# their place, and are in the rules now.
#
# astpp used to be here and is in ../libexec now, with the passes -
# see that directory's makefile.  asz and ld were there too, but they
# are commands a person types directly, to assemble or link, so they
# belong in /bin and are back in the rules now.
#
# cc is left out because it still builds itself with the Whitesmith
# compiler and its -lS; far, msh and the rest were out before that.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# The toolchain: the three programs that must exist before the passes
# are rebuilt.  ccc needs ld to link and asz to assemble, and the
# passes are built with ccc, so the top-level makefile builds these
# three first, then libexec, then the rest of us.  The order here is
# ld, asz, ccc - linker, assembler, driver.
#
toolchain:
	cd ld ; make install
	cd asz ; make install
	cd ccc ; make install

all:
	cd make ; make all
	cd makekey ; make all
	cd ar ; make all
	cd asz ; make all
	cd awk ; make all
	cd cal ; make all
	cd cat ; make all
	cd ccc ; make all
	cd chgrp ; make all
	cd chmod ; make all
	cd chown ; make all
	cd cmp ; make all
	cd comm ; make all
	cd cp ; make all
	cd crypt ; make all
	cd cu ; make all
	cd date ; make all
	cd dc ; make all
	cd dcheck ; make all
	cd dd ; make all
	cd diff ; make all
	cd du ; make all
	cd echo ; make all
	cd ed ; make all
	cd expr ; make all
	cd fd2 ; make all
	cd fgrep ; make all
	cd file ; make all
	cd find ; make all
	cd form ; make all
	cd grep ; make all
	cd group ; make all
	cd icheck ; make all
	cd init ; make all
	cd kill ; make all
	cd label ; make all
	cd ld ; make all
	cd less ; make all
	cd ln ; make all
	cd ls ; make all
	cd mail ; make all
	cd man ; make all
	cd mkdir ; make all
	cd mknod ; make all
	cd more ; make all
	cd mv ; make all
	cd ncheck ; make all
	cd nm ; make all
	cd od ; make all
	cd passwd ; make all
	cd pr ; make all
	cd pwd ; make all
	cd rm ; make all
	cd rmdir ; make all
	cd s ; make all
	cd sed ; make all
	cd setdev ; make all
	cd sh ; make all
	cd size ; make all
	cd sort ; make all
	cd strings ; make all
	cd stty ; make all
	cd su ; make all
	cd sum ; make all
	cd sync ; make all
	cd tail ; make all
	cd tar ; make all
	cd tee ; make all
	cd tty ; make all
	cd unique ; make all
	cd update ; make all
	cd vi ; make all
	cd wall ; make all
	cd wc ; make all
	cd who ; make all
	cd write ; make all
	cd yacc ; make all
	cd lex ; make all

install:
	cd make ; make install
	cd makekey ; make install
	cd ar ; make install
	cd asz ; make install
	cd awk ; make install
	cd cal ; make install
	cd cat ; make install
	cd ccc ; make install
	cd chgrp ; make install
	cd chmod ; make install
	cd chown ; make install
	cd cmp ; make install
	cd comm ; make install
	cd cp ; make install
	cd crypt ; make install
	cd cu ; make install
	cd date ; make install
	cd dc ; make install
	cd dcheck ; make install
	cd dd ; make install
	cd diff ; make install
	cd du ; make install
	cd echo ; make install
	cd ed ; make install
	cd expr ; make install
	cd fd2 ; make install
	cd fgrep ; make install
	cd file ; make install
	cd find ; make install
	cd form ; make install
	cd grep ; make install
	cd group ; make install
	cd icheck ; make install
	cd init ; make install
	cd kill ; make install
	cd label ; make install
	cd ld ; make install
	cd less ; make install
	cd ln ; make install
	cd ls ; make install
	cd mail ; make install
	cd man ; make install
	cd mkdir ; make install
	cd mknod ; make install
	cd more ; make install
	cd mv ; make install
	cd ncheck ; make install
	cd nm ; make install
	cd od ; make install
	cd passwd ; make install
	cd pr ; make install
	cd pwd ; make install
	cd rm ; make install
	cd rmdir ; make install
	cd s ; make install
	cd sed ; make install
	cd setdev ; make install
	cd sh ; make install
	cd size ; make install
	cd sort ; make install
	cd strings ; make install
	cd stty ; make install
	cd su ; make install
	cd sum ; make install
	cd sync ; make install
	cd tail ; make install
	cd tar ; make install
	cd tee ; make install
	cd tty ; make install
	cd unique ; make install
	cd update ; make install
	cd vi ; make install
	cd wall ; make install
	cd wc ; make install
	cd who ; make install
	cd write ; make install
	cd yacc ; make install
	cd lex ; make install

clean:
	cd make ; make clean
	cd makekey ; make clean
	cd ar ; make clean
	cd asz ; make clean
	cd awk ; make clean
	cd cal ; make clean
	cd cat ; make clean
	cd ccc ; make clean
	cd chgrp ; make clean
	cd chmod ; make clean
	cd chown ; make clean
	cd cmp ; make clean
	cd comm ; make clean
	cd cp ; make clean
	cd crypt ; make clean
	cd cu ; make clean
	cd date ; make clean
	cd dc ; make clean
	cd dcheck ; make clean
	cd dd ; make clean
	cd diff ; make clean
	cd du ; make clean
	cd echo ; make clean
	cd ed ; make clean
	cd expr ; make clean
	cd fd2 ; make clean
	cd fgrep ; make clean
	cd file ; make clean
	cd find ; make clean
	cd form ; make clean
	cd grep ; make clean
	cd group ; make clean
	cd icheck ; make clean
	cd init ; make clean
	cd kill ; make clean
	cd label ; make clean
	cd ld ; make clean
	cd less ; make clean
	cd ln ; make clean
	cd ls ; make clean
	cd mail ; make clean
	cd man ; make clean
	cd mkdir ; make clean
	cd mknod ; make clean
	cd more ; make clean
	cd mv ; make clean
	cd ncheck ; make clean
	cd nm ; make clean
	cd od ; make clean
	cd passwd ; make clean
	cd pr ; make clean
	cd pwd ; make clean
	cd rm ; make clean
	cd rmdir ; make clean
	cd s ; make clean
	cd sed ; make clean
	cd setdev ; make clean
	cd sh ; make clean
	cd size ; make clean
	cd sort ; make clean
	cd strings ; make clean
	cd stty ; make clean
	cd su ; make clean
	cd sum ; make clean
	cd sync ; make clean
	cd tail ; make clean
	cd tar ; make clean
	cd tee ; make clean
	cd tty ; make clean
	cd unique ; make clean
	cd update ; make clean
	cd vi ; make clean
	cd wall ; make clean
	cd wc ; make clean
	cd who ; make clean
	cd write ; make clean
	cd yacc ; make clean
	cd lex ; make clean

clobber:
	cd make ; make clobber
	cd makekey ; make clobber
	cd ar ; make clobber
	cd asz ; make clobber
	cd awk ; make clobber
	cd cal ; make clobber
	cd cat ; make clobber
	cd ccc ; make clobber
	cd chgrp ; make clobber
	cd chmod ; make clobber
	cd chown ; make clobber
	cd cmp ; make clobber
	cd comm ; make clobber
	cd cp ; make clobber
	cd crypt ; make clobber
	cd cu ; make clobber
	cd date ; make clobber
	cd dc ; make clobber
	cd dcheck ; make clobber
	cd dd ; make clobber
	cd diff ; make clobber
	cd du ; make clobber
	cd echo ; make clobber
	cd ed ; make clobber
	cd expr ; make clobber
	cd fd2 ; make clobber
	cd fgrep ; make clobber
	cd file ; make clobber
	cd find ; make clobber
	cd form ; make clobber
	cd grep ; make clobber
	cd group ; make clobber
	cd icheck ; make clobber
	cd init ; make clobber
	cd kill ; make clobber
	cd label ; make clobber
	cd ld ; make clobber
	cd less ; make clobber
	cd ln ; make clobber
	cd ls ; make clobber
	cd mail ; make clobber
	cd man ; make clobber
	cd mkdir ; make clobber
	cd mknod ; make clobber
	cd more ; make clobber
	cd mv ; make clobber
	cd ncheck ; make clobber
	cd nm ; make clobber
	cd od ; make clobber
	cd passwd ; make clobber
	cd pr ; make clobber
	cd pwd ; make clobber
	cd rm ; make clobber
	cd rmdir ; make clobber
	cd s ; make clobber
	cd sed ; make clobber
	cd setdev ; make clobber
	cd sh ; make clobber
	cd size ; make clobber
	cd sort ; make clobber
	cd strings ; make clobber
	cd stty ; make clobber
	cd su ; make clobber
	cd sum ; make clobber
	cd sync ; make clobber
	cd tail ; make clobber
	cd tar ; make clobber
	cd tee ; make clobber
	cd tty ; make clobber
	cd unique ; make clobber
	cd update ; make clobber
	cd vi ; make clobber
	cd wall ; make clobber
	cd wc ; make clobber
	cd who ; make clobber
	cd write ; make clobber
	cd yacc ; make clobber
	cd lex ; make clobber
