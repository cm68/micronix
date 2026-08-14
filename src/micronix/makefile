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
# and /lib/libu.a from paths it works out from its own, so what cmd
# links against is what lib has already put on the system, not
# anything in this tree.  The same goes for the headers, which cpp
# reads out of /usr/include.
#
#	include		the headers, before anything compiles
#	lib		libc, libu, libutil, libccc and crt0
#	libexec		the passes - and they link against libccc
#	cmd		the commands, and ccc among them
#	sys		the kernel
#
# libexec was missing from this list entirely, so a native build never
# built the compiler it was running.
#
all:
	cd include ; make install
	cd lib ; make install
	cd libexec ; make install
	cd cmd ; make install
	cd sys ; make install

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

#
# There was an "upgrade" here which ran "make upgrade" in include, lib
# and cmd.  Only include had such a rule and it did what install does
# now; the other two never had one, and this make does not look at the
# status of a command it runs - see the loop at the end of make.c,
# which calls docmd and throws the answer away - so the target had
# been printing two errors and reporting success for some years.
#
