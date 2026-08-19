#
# makefile for nm
#
# micronix/cmd/nm/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# Lists the symbols in an object, and disassembles it with -d.
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
MAN	= nm.1
MANDIR	= /usr/man/man1
PROG	= nm
OBJS	= nm.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) /lib/libutil.a

install: $(PROG)
	cp $(PROG) $(BIN)/nm
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
