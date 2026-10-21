#
# the native build: in the simulator, and on the hardware
#
# micronix/makefile
#
# There are two makefiles here and only two.  This one is read by the
# make that runs on micronix; GNUmakefile beside it is the cross build
# and is read on the host.  Nothing has to look at which make it is
# running any more.
#
# There used to be a third - a Makefile, capital M, which the OLD make
# preferred and which existed to bootstrap the new one by building
# cmd/Make first.  That is retired: the cross compiler works, so a
# system without a make gets one built for it on the host rather than
# having to climb out of the hole by itself.  Make is in ../attic with
# the other two that lost the name.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# THE ORDER IS THE BOOTSTRAP, and every rule here builds AND installs
# for that reason.  A command's makefile says "ccc -o sh sh.o" and
# names no library at all: the driver adds /lib/crt0.o, /lib/libc.a
# and /lib/libunix.a from paths it works out from its own, so what cmd
# links against is what lib has already put on the system, not
# anything in this tree.  The same goes for the headers, which cpp
# reads out of /usr/include.
#
#	include		the headers, before anything compiles
#	lib		libc, libunix, libutil, libccc and crt0
#	cmd/toolchain	ld, asz and ccc - linker, assembler and driver
#	libexec		the passes - cpp, c0, c1, peep, astpp
#	cmd		the commands, everything else
#	sys		the kernel
#
# The build assumes a working toolchain is already installed - the seed
# - and replaces it from the bottom up.
#
# libexec was missing from this list entirely, so a native build never
# built the compiler it was running.  It once ran ahead of cmd too, so
# a changed ccc never propagated into the passes it drives.
#
all:
	cd include ; make install
	cd lib ; make install
	cd cmd ; make toolchain
	cd libexec ; make install
	cd cmd ; make install
	
# cd sys ; make install

#
# build - the second generation: build everything and install nothing.
# After clobber, this compiles the tree with the toolchain the previous
# pass installed.  all cannot serve here because all installs as it
# goes, which would replace the copies verify wants to compare against.
#
build:
	cd lib ; make all
	cd libexec ; make all
	cd cmd ; make all

#
# install - the same steps as all, without the bootstrap ordering.  all
# already installs; this is here so the pair the verify test names -
# make, make install, make clobber, make build, make verify - reads the
# way it runs.
#
install:
	cd include ; make install
	cd lib ; make install
	cd libexec ; make install
	cd cmd ; make install

#
# verify - the inverse of install.  Every directory compares each
# artifact it just built against the copy the first generation
# installed; a difference is what a second generation that is not the
# same compiler would produce.
#
verify:
	cd include ; make verify
	cd lib ; make verify
	cd libexec ; make verify
	cd cmd ; make verify

clean:
	cd include ; make clean
	cd lib ; make clean
	cd libexec ; make clean
	cd cmd ; make clean
	cd sys ; make clean

clobber: clean
	cd include ; make clobber
	cd lib ; make clobber
	cd libexec ; make clobber
	cd cmd ; make clobber
	cd sys ; make clobber

