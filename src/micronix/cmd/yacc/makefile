#
# makefile for yacc
#
# micronix/cmd/yacc/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same programs on the host.
#
# Two products: yacc, and liby.a which is linked into the parser yacc
# generates.  The templates yaccopar, yaccepar and yaccrpar are copied
# into that parser too, so they are installed beside the library.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
LIBDIR	= /lib
AUXDIR	= /usr/lib
MAN	= yacc.1
MANDIR	= /usr/man/man1
PROG	= yacc
OBJS	= y0.o y1.o y2.o y3.o y4.o y5.o
LIBOBJS	= libmai.o libpar.o libzac.o libzer.o libzin.o
HDRS	= dextern files

all: $(PROG) liby.a

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

liby.a: $(LIBOBJS)
	rm -f $@
	ar r $@ $(LIBOBJS)

$(OBJS) $(LIBOBJS): $(HDRS)

install: $(PROG) liby.a
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	mkdir -p $(LIBDIR)
	cp liby.a $(LIBDIR)/liby.a
	mkdir -p $(AUXDIR)
	cp yaccopar yaccepar yaccrpar $(AUXDIR)/
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) $(LIBOBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG) liby.a
