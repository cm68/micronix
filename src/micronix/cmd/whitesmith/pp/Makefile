#
# native Makefile for whitesmiths cpp
#
# src/cmd/whitesmith/pp
#
# Changed: <2023-07-10 17:58:44 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: pp

PPOBJS = p0eval.o p0lex.o p0main.o p0subs.o

pp: $(PPOBJS)
	cc -o pp $(PPOBJS) -lws

$(PPOBJS) : $*.c
	cc -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f pp
