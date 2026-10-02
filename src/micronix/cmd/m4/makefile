#
# makefile for m4
#
# micronix/cmd/m4/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# -Dunix turns on SIGHUP/SIGINT in <signal.h> and the "unix" predefined
# macro; the M4 variant is left off.  m4eval.c is the hand-written eval
# parser, m4y.y the original yacc grammar.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O -Dunix
BIN	= /bin
MAN	= m4.1
MANDIR	= /usr/man/man1
PROG	= m4
OBJS	= m4.o m4eval.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
