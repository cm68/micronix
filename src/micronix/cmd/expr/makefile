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
# native build does not have to run yacc first.  expr links -ly for
# yyerror; liby.a's yyparse and main members are not pulled because
# y.tab.o already defines both symbols.
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
	$(CC) $(CFLAGS) -o $@ $(OBJS) -ly

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
