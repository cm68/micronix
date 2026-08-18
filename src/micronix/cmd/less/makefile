#
# makefile for less
#
# micronix/cmd/less/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
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
MAN	= less.1
MANDIR	= /usr/man/man1
LIB	= /usr/lib
PROG	= less
HDRS	= less.h position.h funcs.h
OBJS	= main.o option.o prim.o ch.o position.o input.o output.o \
	screen.o prompt.o line.o signal.o help.o ttyin.o command.o \
	version.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $(HDRS)

#
# the help file rides along: the h command runs another less over it.
#
install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)
	cp $(MAN) $(MANDIR)/$(MAN)
	cp less.help $(LIB)/less.help

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
