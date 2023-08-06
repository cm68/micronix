#
# micronix makefile for rm
#
# this lower case file is preferred by the newer make
#
# micronix/cmd/rm/makefile
#
# Changed: <2023-07-27 12:13:40 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	rm

rm : rm.c
	cc -i /usr/include/ -o rm rm.c -lS -lS

upgrade: rm
	cp /bin/rm /old/rm
	cp rm /bin/rm

install: rm
	cp rm /bin/rm

clean:
	rm -f *.o

clobber: clean
	rm -f rm
