#
# makefile for ls
#
# micronix/cmd/ls/Makefile
#
# Changed: <2023-07-27 10:20:23 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	ls

ls : ls.c
	cc -I/usr/include -o ls ls.c -lS

install:
	cp ls /bin/ls

upgrade: ls
	cp /bin/ls /old/ls
	cp ls /bin/ls

clean: ; rm ls *.o

