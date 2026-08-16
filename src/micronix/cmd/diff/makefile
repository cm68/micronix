#
# makefile for diff
#
# micronix/cmd/diff/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# Ported from 2.11BSD usr/src/bin/diff.  diffh is not ported, so -h
# is accepted and falls through to the full algorithm; on a machine
# whose files fit in 64k that is not the loss it is on a PDP-11.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
BIN	= /bin
PROG	= diff
OBJS	= diff.o diffdir.o diffreg.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
