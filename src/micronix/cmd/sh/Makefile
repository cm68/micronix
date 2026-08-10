#
# makefile for sh
#
# micronix/cmd/sh/Makefile
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJS = sh.o parse.o

all:	sh

sh : $(OBJS)
	cc -o sh $(OBJS) -lS

sh.o : sh.c sh.h
	cc -I/usr/include -c sh.c

parse.o : parse.c sh.h
	cc -I/usr/include -c parse.c

# a host build, for comparing behaviour against the original binary
# running under the simulator
host : sh.c parse.c sh.h
	gcc -std=gnu89 -w -o sh.host sh.c parse.c

install: sh
	cp sh /bin/nsh

clean: ; /bin/rm -f sh sh.host *.o
