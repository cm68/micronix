#
# native Makefile for pp
#
# pp/Makefile
#
# Changed: <2023-07-05 22:25:01 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: pp

pp: p0main.o p0eval.o p0lex.o p0subs.o
	cc -v -o pp p0main.o p0eval.o p0lex.o p0subs.o -lws -lws -lS

p0main.o:	p0main.c
	cc -c p0main.c

p0eval.o:	p0eval.c
	cc -c p0eval.c

p0lex.o:	p0lex.c
	cc -c p0lex.c

p0subs.o:	p0subs.c
	cc -c p0subs.c

test:	pp
	./pp -d unix -i /include/ -x -o p0subs.i p0subs.c

clean:
	rm *.o

clobber: clean
	rm -f pp

