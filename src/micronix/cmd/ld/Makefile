all:	ld

ld : ld.c
	cc -o ld ld.c -lS

upgrade: ld
	cp /bin/ld /old/ld

install: ld
	cp ld /bin/ld

clean: ; rm ld *.o
