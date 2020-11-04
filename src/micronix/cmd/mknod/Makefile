all:	mknod

mknod : mknod.c
	cc -o mknod mknod.c -lS

install: mknod
	cp mknod /bin/mknod

clean: ; rm mknod *.o
