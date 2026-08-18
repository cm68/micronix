#
# makefile for lex
#
# micronix/cmd/lex/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same programs on the host.
#
# Two products: lex, and liblc.a which is linked into the scanner lex
# generates.  ncform is copied into that scanner too, so it is
# installed beside the library.  lex links -ly for yyerror; liby.a's
# yyparse and main members are not pulled because lex already defines
# both symbols.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O -Dunix
BIN	= /bin
LIBDIR	= /lib
AUXDIR	= /usr/lib/lex
MAN	= lex.1
MANDIR	= /usr/man/man1
PROG	= lex
OBJS	= main.o header.o sub1.o sub2.o y.tab.o
LIBOBJS	= libmai.o allprint.o yyless.o yywrap.o ereject.o
HDRS	= ldefs.c once.c

all: $(PROG) liblc.a

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -ly

liblc.a: $(LIBOBJS)
	rm -f $@
	ar r $@ $(LIBOBJS)

$(OBJS) $(LIBOBJS): $(HDRS)

install: $(PROG) liblc.a
	cp $(PROG) $(BIN)/$(PROG)
	cp liblc.a $(LIBDIR)/liblc.a
	mkdir -p $(AUXDIR)
	cp ncform $(AUXDIR)/ncform
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) $(LIBOBJS)

clobber: clean
	rm -f $(PROG) liblc.a
