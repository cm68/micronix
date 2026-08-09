#
# makefile for mkfs
#
# micronix/cmd/mkfs/Makefile
#

all: mkfs

mkfs: mkfs.c
	cc -I/usr/include -o mkfs mkfs.c -lS

install: mkfs
	cp mkfs /bin/mkfs

upgrade: mkfs
	cp /bin/mkfs /old/mkfs
	cp mkfs /bin/mkfs

clean:
	rm mkfs *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
