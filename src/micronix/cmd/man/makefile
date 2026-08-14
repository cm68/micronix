#
# makefile for man
#
# micronix/cmd/man/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The manual reader.  It formats a page by exec'ing form(1) - there
# are no macros in this world - and installs two pages, its own
# and help's.
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
MAN	= man.1 help.1
MANDIR	= /usr/man/man1
PROG	= man
OBJS	= man.o

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
	cp $(PROG) $(BIN)/man
	cp $(MAN) $(MANDIR)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
