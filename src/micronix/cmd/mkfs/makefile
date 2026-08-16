#
# makefile for mkfs
#
# micronix/cmd/mkfs/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# Was a Makefile, capital M, and carried both halves itself: an "all"
# that called the whitesmiths cc with -lS and a separate "cross" that
# called ccc with an -i for the host's include directory.  The two
# builds are two files now, which is what the rest of the tree does,
# and this one is the native half.
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
MAN	= mkfs.1
MANDIR	= /usr/man/man1
PROG	= mkfs
OBJS	= mkfs.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
