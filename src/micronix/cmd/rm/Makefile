#
# makefile for rm
#
# micronix/cmd/rm/Makefile
#
# Changed: <2023-07-10 15:35:12 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	rm

rm : rm.c
	cc -I/usr/include -o rm rm.c -lS

install: rm
	cp rm /bin/rm

upgrade: rm
	cp /bin/rm /old/rm
	cp rm /bin/rm

clean: ; /bin/rm rm *.o

