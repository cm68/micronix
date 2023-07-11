#
# makefile for cp
#
# micronix/cmd/cp/Makefile
#
# Changed: <2023-07-10 15:35:25 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	cp

cp : cp.c
	cc -I/usr/include -o cp cp.c -lS

upgrade: cp
	./cp /bin/cp /old/cp
	./cp cp /bin/cp

install: cp
	./cp cp /bin/cp

clean: ; /bin/rm cp *.o
