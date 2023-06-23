#
# makefile for ld
#
# micronix/cmd/ld/Makefile
#
# Changed: <2023-06-23 01:11:42 curt>


all:	ld

ld : ld.c
	cc -o ld ld.c -lS

upgrade: ld
	cp /bin/ld /old/ld

install: ld
	cp ld /bin/ld

clean: ; rm ld *.o

# vim: tabstop=4 shiftwidth=4 expandtab:
