#
# micronix native standalone
#
# micronix/stand/Makefile
#
# Changed: <2023-07-10 14:38:40 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mwboot.com mwboot1.cim

mwio.o:
	cc -i /include/ -c mwio.c

boot.o:
	cc -i /include/ -c boot.c

#
# The first level is a sector, not a program: the rom reads 512 bytes to
# 0100 and jumps, so there is nowhere to put a header and nothing to
# strip one.  zmac's cim output is the flat image that wants, which is
# why this one is assembled on the host rather than by as and link.
#
mwboot1.cim: mwboot1.s
	zmac mwboot1.s
	cp zout/mwboot1.cim .

#
# what mkfs -i writes: the first level in block 0, the second level from
# block 1, which is where the first level looks for it.
#
bootimg: mwboot1.cim mwboot.com
	dd if=mwboot1.cim of=bootimg bs=512 conv=sync
	cat mwboot.com >>bootimg

mwboot.com: boot.o mwio.o
	link -tb0x100 -tr -dr12 -h -o mwboot.com /lib/uhdr.o boot.o mwio.o -lS.a -lm.a
