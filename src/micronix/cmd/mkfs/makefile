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
CFLAGS	=
BIN	= /bin
PROG	= mkfs
OBJS	= mkfs.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
