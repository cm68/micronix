#
# native Makefile for whitesmiths unix system call library
#
# src/cmd/whitesmith/lib/libu/Makefile
#
# Changed: <2023-09-04 08:27:00 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

LIB = libu.a

all: libu.a

install: $(LIB)
	cp $(LIB) /lib

SOBJS =

COBJS =

LIBOBJS = $(SOBJS) $(COBJS)

libu.a: $(LIBOBJS)
	lib libu.a -cv $(LIBOBJS)

$(SOBJS) : $*.s
	as -o $*.o $*.s

$(COBJS) : $*.c
	cc -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f libu.a

