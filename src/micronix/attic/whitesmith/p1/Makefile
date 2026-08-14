#
# native Makefile for whitesmiths p1
#
# src/cmd/whitesmith/p1
#
# Changed: <2023-07-10 18:06:37 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I/usr/src/cmd/whitesmith/include

all: p1

install: p1
	cp p1 /bin/cp1

P1OBJS = p1data.o p1expr.o p1func.o p1gets.o p1main.o\
	p1puts.o p1red.o p1subs.o p1type.o

p1: $(P1OBJS)
	cc -o p1 $(P1OBJS) -lws

$(P1OBJS) : $*.c
	cc $(INCLUDE) -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f p1
