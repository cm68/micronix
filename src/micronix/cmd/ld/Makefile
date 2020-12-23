all:	ld

ld : ld.c
	cc -o ld ld.c -lS

install: ld /old/ld
	cp ld /bin/ld

/old/ld:
	cp /bin/ld /old/ld

clean: ; rm ld *.o
