#
# makefile for ls
#
# micronix/cmd/ls/Makefile
#
# Changed: <2023-07-10 15:33:57 curt>

all:	ls

ls : ls.c
	cc -I/usr/include -o ls ls.c -lS

install:
	cp ls /bin/ls

upgrade: ls
	cp /bin/ls /old/ls
	cp ls /bin/ls

clean: ; rm ls *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
