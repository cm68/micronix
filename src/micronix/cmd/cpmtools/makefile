#
# makefile for formatmw
#
# micronix/cmd/cpmtools/makefile
#
# The native build.  Was a Makefile, capital M - the name the old
# make preferred, which nothing uses now.
#
# formatmw is a CP/M program and not a micronix one: it was built
# with "cc -o formatmw.com -lwsc -cpm", the whitesmiths compiler
# targeting CP/M.  ccc has a cpm target of its own, which is what -m
# selects, so that is what this asks for.  Nothing in the tree builds
# this directory and it is not in DIRS.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -m cpm
PROG	= formatmw.com
OBJS	= formatmw.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install:

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
