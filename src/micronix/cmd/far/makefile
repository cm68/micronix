#
# makefile for far
#
# micronix/cmd/far/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.
#
# This was a Makefile, capital M, which is the name the OLD make
# preferred and which nothing uses now - the top level dropped its
# own for the same reason.  It called the whitesmiths cc with -cz and
# linked -lS, none of which is how this tree builds any more; ccc
# links libc by itself.
#
# far is not in DIRS and its GNUmakefile does not build it either.
# This is here so that the directory says how, in the form the rest
# of the tree says it.
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
PROG	= far
HDRS	= far.h
OBJS	= far1.o far2.o far3.o far4.o far5.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $(HDRS)

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
