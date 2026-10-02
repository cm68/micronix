#
# makefile for cpp
#
# micronix/cmd/cpp/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# cpp is the STANDALONE C preprocessor, a filter - not a pass of the
# compiler, whose front end is /libexec/pass0.  See the README.
#
# yylex.c is #included by cpy.c, so it is a dependency of cpy.o but is
# never compiled as a unit of its own.
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
MAN	= cpp.1
MANDIR	= /usr/man/man1
PROG	= cpp
OBJS	= cpp.o cpy.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

cpy.o: yylex.c

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
