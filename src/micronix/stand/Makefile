#
# micronix native standalone
#
# micronix/stand/Makefile
#
# Changed: <2023-07-10 14:38:40 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mwboot.com

mwio.o:
	cc -i /include/ -c mwio.c

boot.o:
	cc -i /include/ -c boot.c

mwboot.com: boot.o mwio.o
	link -tb0x100 -tr -dr12 -h -o mwboot.com /lib/uhdr.o boot.o mwio.o -lS.a -lm.a
