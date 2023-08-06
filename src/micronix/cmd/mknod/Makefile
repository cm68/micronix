#
# makefile for mknod
#
# micronix/cmd/mknod/Makefile
#
# Changed: <2023-06-23 01:12:17 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mknod

mknod : mknod.c
	cc -o mknod mknod.c -lS

upgrade: mknod
	cp /bin/mknod /old/mknod
	cp mknod /bin/mknod

install: mknod
	cp mknod /bin/mknod

clean: ; rm mknod *.o

