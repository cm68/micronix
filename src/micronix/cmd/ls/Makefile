#
# makefile for ls
#
# micronix/cmd/ls/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

all:	ls

ls : ls.c
	cc -o ls ls.c -lS

install:
	cp ls /bin/ls

upgrade: ls
	cp /bin/ls /old/ls
	cp ls /bin/ls

clean: ; rm ls *.o

# vim: tabstop=4 shiftwidth=4 expandtab:
