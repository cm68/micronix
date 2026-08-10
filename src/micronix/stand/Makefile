#
# micronix native standalone
#
# micronix/stand/Makefile
#
# Changed: <2023-07-10 14:38:40 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mwboot.com mwboot1.bin

mwio.o:
	cc -i /include/ -c mwio.c

boot.o:
	cc -i /include/ -c boot.c

#
# ccc's tools, which live beside its passes rather than on the path.
#
CCCLIB = /usr/local/lib
ASZ = $(CCCLIB)/asz
WSLD = $(CCCLIB)/wsld

#
# The first level is a sector, not a program: the rom reads 512 bytes to
# 0100 and jumps, so there is nowhere to put a header and nothing to
# strip one.
#
# asz has no org - it writes a relocatable object and the linker says
# where the text goes - so the 0100 is given here, and mwboot1.s says
# the same number once, in BASE, for the two addresses it has to work
# out itself.  The two have to agree.
#
# -s because an unstripped link puts the symbol table after the text and
# all of it would be in the file we are about to call a sector.  Then
# the 16 byte header comes off the front - one block of it, which is
# what makes bs=16 skip=1 the whole of the strip.
#
mwboot1.bin: mwboot1.s
	$(ASZ) mwboot1.s
	$(WSLD) -s -Ttext=0x100 -o mwboot1.abs mwboot1.o
	dd if=mwboot1.abs of=mwboot1.bin bs=16 skip=1

#
# what mkfs -i writes: the first level in block 0, the second level from
# block 1, which is where the first level looks for it.
#
bootimg: mwboot1.bin mwboot.com
	dd if=mwboot1.bin of=bootimg bs=512 conv=sync
	cat mwboot.com >>bootimg

mwboot.com: boot.o mwio.o
	link -tb0x100 -tr -dr12 -h -o mwboot.com /lib/uhdr.o boot.o mwio.o -lS.a -lm.a
