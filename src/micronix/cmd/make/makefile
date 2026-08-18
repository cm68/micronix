#
# makefile for make
#
# micronix/cmd/make/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# y.tab.c is the parser yacc generated from gram.y, committed so the
# native build does not have to run yacc first.  defs.h is the shared
# header every .c includes.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= make.1
MANDIR	= /usr/man/man1
PROG	= make
OBJS	= main.o doname.o misc.o files.o dosys.o y.tab.o ident.o
HDRS	= defs.h

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $(HDRS)

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
