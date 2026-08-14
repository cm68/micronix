#
# makefile for man
#
# micronix/cmd/man/Makefile
#
# The native build, run on Micronix by the tools already there.  The
# cross build is GNUmakefile beside this.

all: man

man: man.c
	cc -I/usr/include -o man man.c -lS

install: man
	cp man /bin/man
	rm -f /bin/help
	ln /bin/man /bin/help

clean:
	rm -f man *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
