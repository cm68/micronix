#
# makefile for expr
#
# micronix/cmd/expr/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# y.tab.c is the parser yacc generated from expr.y, committed so the
# native build does not have to run yacc first.  expr is
# self-contained - yyinit and yyerror are inlined in expr.y - so it
# does not link -ly.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= expr.1
MANDIR	= /usr/man/man1
PROG	= expr
OBJS	= y.tab.o

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

#
# verify - the inverse of install: every installed artifact against
# the one this build just produced.  Run it after a clobber and a
# second build to prove the rebuild matches what the first
# generation installed.
#
verify: $(PROG)
	cmp $(PROG) $(BIN)/$(PROG)
	cmp $(MAN) $(MANDIR)/$(MAN)
