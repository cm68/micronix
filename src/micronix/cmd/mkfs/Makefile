#
# makefile for mkfs
#
# micronix/cmd/mkfs/Makefile
#

all: mkfs

mkfs: mkfs.c
	cc -I/usr/include -o mkfs mkfs.c -lS

#
# Cross building with ccc needs one include directory holding both sets
# of headers, because -i replaces the system include directory rather
# than adding to it: ccc's own stdio.h and the rest of its libc, with
# micronix/include's types.h and sys/{fs,dir,stat}.h over the top of
# them.  Point INC at such a directory.
#
#	make cross INC=/somewhere/merged-include
#
INC = /usr/local/lib/include

cross: mkfs.c
	ccc -i$(INC) -o mkfs mkfs.c

install: mkfs
	cp mkfs /bin/mkfs

upgrade: mkfs
	cp /bin/mkfs /old/mkfs
	cp mkfs /bin/mkfs

clean:
	rm mkfs *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
