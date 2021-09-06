all:	mwboot.com

mwio.o:
	cc -i ../include/ -c mwio.c

boot.o:
	cc -i ../include/ -c boot.c

mwboot.com: boot.o mwio.o
	link -tb0x100 -tr -dr12 -h -o mwboot.com /lib/uhdr.o boot.o mwio.o -lS.a -lm.a
