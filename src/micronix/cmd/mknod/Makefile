all:	mknod

mknod : mknod.c
	cc -o mknod mknod.c -lS

install: mknod /old/mknod
	cp mknod /bin/mknod

/old/mknod:
	cp /bin/mknod /old/mknod

clean: ; rm mknod *.o
