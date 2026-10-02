#
# makefile for ratfor
#
# micronix/cmd/ratfor/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# -Dunix is what sets CONTFLD 1 in r.h.  y.tab.c is generated from r.g
# by yacc and committed, so nothing here runs yacc.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O -Dunix
BIN	= /bin
MAN	= ratfor.1
MANDIR	= /usr/man/man1
PROG	= ratfor
OBJS	= r0.o r1.o r2.o rio.o rlex.o rlook.o y.tab.o
HDRS	= r.h y.tab.h

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $(HDRS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
