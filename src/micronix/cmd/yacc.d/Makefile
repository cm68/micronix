INCLUDE=-I../../include/

YNAME=/bin/yacc
YLIB=/lib/liby.a
AUXDIR=/usr/lib

CFLAGS=-O
LDFLAGS = -s

head: yacc liby.a
	;

OBJECTS = y0.o y1.o y2.o y3.o y4.o y5.o
LIBOBJS = libmai.o libpar.o libzac.o libzer.o libzin.o

yacc: $(OBJECTS)
	cc $(LDFLAGS) y0.o y1.o y2.o y3.o y4.o y5.o -lS -o yacc

# y0.o y1.o y2.o y3.o y4.o y5.o: dextern files

liby.a: $(LIBOBJS)
	rm liby.a
	lib liby.a -r -v6 -v $(LIBOBJS)

install: head
	cp yacc $(YNAME)
	cp liby.a $(YLIB)
	cp yaccopar $(AUXDIR)/yaccopar

clobber:
	-rm yacc liby.a
	-rm *.o


$(OBJECTS) $(LIBOBJS) : $*.c
	cc -c $(INCLUDE) $*.c

# vim: tabstop=4 shiftwidth=4 noexpandtab:

