#
# makefile for ld
#
# micronix/cmd/ld/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The link editor.  ccc runs it to finish a link, and it is the one a
# person types to link by hand - so it goes into /bin, on the path,
# beside the driver and the assembler.
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
MAN	= ld.1
MANDIR	= /usr/man/man1
BIN	= /bin
PROG	= ld
OBJS	= ld.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) /lib/libutil.a

install: $(PROG)
	cp $(PROG) $(BIN)/ld
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
