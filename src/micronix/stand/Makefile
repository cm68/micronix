#
# micronix native standalone
#
# micronix/stand/Makefile
#
# Changed: <2021-12-23 15:51:54 curt>

all:	mwboot.com

mwio.o:
	cc -i ../include/ -c mwio.c

boot.o:
	cc -i ../include/ -c boot.c

mwboot.com: boot.o mwio.o
	link -tb0x100 -tr -dr12 -h -o mwboot.com /lib/uhdr.o boot.o mwio.o -lS.a -lm.a

# vim: tabstop=4 shiftwidth=4 expandtab:

