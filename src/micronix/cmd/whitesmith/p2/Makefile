#
# native Makefile for whitesmiths p2
#
# src/cmd/whitesmith/p2
#
# Changed: <2023-07-28 20:15:54 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I/usr/src/cmd/whitesmith/include

all: p2

P2OBJS = p2expr.o p2fmov.o p2gen.o p2jump.o p2main.o p2subs.o p2tab.o q2dbug.o

p2: $(P2OBJS)
	cc -o p2 $(P2OBJS) -lws

$(P2OBJS) : $*.c
	cc $(INCLUDE) -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f p2
