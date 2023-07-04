#
# makefile for rm
#
# micronix/cmd/rm/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

all:	rm

rm : rm.c
	cc -o rm rm.c -lS

install: rm
	cp rm /bin/rm

upgrade: rm
	cp /bin/rm /old/rm
	cp rm /bin/rm

clean: ; /bin/rm rm *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
