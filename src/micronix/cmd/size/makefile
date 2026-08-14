#
# makefile for size
#
# micronix/cmd/size/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# Prints the segment sizes of an object.
#
# The object list is spelled out because this make has no wildcard.
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
PROG	= size
HDRS	= wsobj.h
OBJS	= size.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c $(HDRS)
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/size

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
