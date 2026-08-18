#
# makefile for diff and diffh
#
# micronix/cmd/diff/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same programs on the host.
#
# diffh is the halfhearted diff that diff's -h execs.  It is a second
# program built here, with its own object, so it is not part of
# $(OBJS).
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
MAN	= diff.1
MANDIR	= /usr/man/man1
PROG	= diff
OBJS	= diff.o diffdir.o diffreg.o

all: $(PROG) diffh

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

diffh: diffh.o
	$(CC) $(CFLAGS) -o $@ diffh.o

install: $(PROG) diffh
	cp $(PROG) $(BIN)/$(PROG)
	cp $(MAN) $(MANDIR)/$(MAN)
	cp diffh $(BIN)/diffh

clean:
	rm -f $(OBJS) diffh.o

clobber: clean
	rm -f $(PROG) diffh
