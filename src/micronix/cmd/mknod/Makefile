all:	mknod

mknod : mknod.c
	cc -o mknod mknod.c -lS

upgrade: mknod
	cp /bin/mknod /old/mknod
	cp mknod /bin/mknod

install: mknod
	cp mknod /bin/mknod

clean: ; rm mknod *.o
