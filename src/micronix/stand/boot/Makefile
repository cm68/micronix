#
# micronix native standalone
#
# micronix/stand/boot/Makefile
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mwboot.com mwboot1.bin bootimgs djboot.com djboot1.bin djload \
	ideboot.com ideboot1.bin idebootimgs \
	ncrboot.com ncrboot1.bin ncrbootimgs

install:

#
# Every name is written out, the bootimg-* among them from $(BOOTIMGS),
# which is the same list the rule that makes them reads.  A glob is the
# same names only for as long as this file and it agree; a list cannot
# disagree with itself.  The micronix-side makefiles follow the same
# rule for a harder reason - see sys/Makefile, where "rm -f *.o" came to
# more words than the shell would take, so the rm never ran at all and
# the names that did not fit were read back as a command of their own.
#
# Four entries are gone rather than enumerated.  *.cim, *.hex and zout
# are the Whitesmiths build's: nothing here has produced them since
# mwboot1.cim stopped being cat'd in front of mwboot.com, and zout is a
# directory of that assembler's listings.  core.* is a crash dump - not
# a build product, and named core.<pid>, so there is no list to write.
clean:
	rm -f $(OBJS) djio.o ideio.o ncrio.o *.nam *.tok *.x
	rm -f ideboot.com djboot.com ncrboot.com
	rm -f mwboot1.abs djboot1.abs mwboot1.bin djboot1.bin
	rm -f ideboot1.abs ideboot1.bin ncrboot1.abs ncrboot1.bin
	rm -f mwboot.com djload mkbootimg
	rm -f $(BOOTIMGS) $(IDEBOOTIMGS) $(NCRBOOTIMGS) bootimg

clobber: clean

#
# The second level is built with ccc, which cross builds here instead of
# needing the simulator to compile it.
#
# -S because mwboot1.s reads NSEC sectors and no more - 8 of them, 4k -
# and a symbol table the loader never looks at would push it past that.
# 3k of program with 1.5k of symbols after it does not fit in 4k.
#
# INC is one directory holding both sets of headers, because -i replaces
# the system include directory rather than adding to it: ccc's own libc
# headers with micronix/include's types.h, obj.h and sys/* over them.
# Same arrangement as cmd/mkfs.
#
#	make INC=/somewhere/merged-include
#
CCC = ccc
#
# One directory holding both sets of headers: ccc's own libc headers
# with micronix/include's types.h, obj.h and sys/* over them, which is
# what "make sysinstall" at the top of micronix produces.  It used to
# say lib/include, which has ccc's and not the system's, so this would
# not compile without being told otherwise on the command line.
#
INC = /usr/local/usr/include

#
# ccc's tools, which live beside its passes rather than on the path.
#
CCCLIB = /usr/local/lib
ASZ = $(CCCLIB)/asz
WSLD = $(CCCLIB)/wsld
MXLD = mxld

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

# djboot1.s is the floppy's first stage, and on the DJ-DMA it is two
# levels in one file.  The firmware copies only 128 bytes to 0080, so
# the file leads with a 128-byte level 0 that does nothing but read the
# next block and jump into it; the level 1 behind it is mwboot1.s with a
# channel program for a read instead of the HD-DMA command block.  The
# link base is therefore 0080, not 0100: level 1's labels are counted
# from there, and they come out at 0100 - where level 0 puts the block -
# only because level 0 and its pad are exactly 128 bytes.  djload checks
# that, since nothing else about the image would look wrong if it were
# not.
djboot1.bin: djboot1.s
	$(ASZ) djboot1.s
	$(WSLD) -s -Ttext=0x80 -o djboot1.abs djboot1.o
	dd if=djboot1.abs of=djboot1.bin bs=16 skip=1

# ideboot1.s is the same sector again, for the IDE card.  It writes the
# drive's task file and moves the sector through the 8255 a word at a
# time instead of handing the HD-DMA a command block; everything else is
# mwboot1.s.
ideboot1.bin: ideboot1.s
	$(ASZ) ideboot1.s
	$(WSLD) -s -Ttext=0x100 -o ideboot1.abs ideboot1.o
	dd if=ideboot1.abs of=ideboot1.bin bs=16 skip=1

# ncrboot1.s is the same sector again, for the NCR 5380.  It differs from
# ideboot1.s in the read: the IDE card's registers are behind an 8255,
# and the 5380 is driven a byte at a time through a REQ/ACK handshake, so
# every sector here is its own command - select the target, send a
# READ(10), take the data, then the status and the message.  It has the
# same 256 byte budget as ideboot1.s, because the label sits at byte 256
# of the sector and mkbootimg refuses a first level that runs into it.
ncrboot1.bin: ncrboot1.s
	$(ASZ) ncrboot1.s
	$(WSLD) -s -Ttext=0x100 -o ncrboot1.abs ncrboot1.o
	dd if=ncrboot1.abs of=ncrboot1.bin bs=16 skip=1

#
# what mkfs -i writes: the first level in block 0, the second level from
# block 1, which is where the first level looks for it.
#
# The second half of that first block is not the first level's: it holds
# a struct dlabel - the geometry, where the boot area starts, and the
# rotation mw.c applies.  On a hard disk that label is the drive's and
# not the image's: mkfs takes the shape from the device node it is given
# rather than from the boot file, and writes the label into the block it
# puts the boot in.  So the half is left empty here for it to fill, and
# there is one image, good for every drive.
#
# The drives a label can describe, from specs[] in sys/mw.c, which is
# the table cmd/mkfs carries too - name:tracks:heads:sectors:
#
#	153 4 17	10404	 5 meg
#	306 4 17	20808	10 meg
#	306 6 17	31212	16 meg
#	640 6 17	65280	32 meg
#	733 5 17	62305	40 meg
#
BOOTIMGS = bootimg

bootimgs: $(BOOTIMGS)

#
# A host program, and it includes the target's sys/dlabel.h so the
# structure it writes cannot drift from the one mkfs writes.
#
mkbootimg: mkbootimg.c
	cc -o mkbootimg mkbootimg.c

bootimg: mwboot1.bin mwboot.com mkbootimg
	./mkbootimg mwboot1.bin mwboot.com $@

#
# A second image for the IDE card, and a separate file rather than the
# same one built twice: what goes in it is ideboot1/ideboot, which drive
# the 8255, and an image is a boot for one card.  One image, though, for
# the reason above - the label is the drive's and mkfs writes it.
#
IDEBOOTIMGS = idebootimg

idebootimgs: $(IDEBOOTIMGS)

idebootimg: ideboot1.bin ideboot.com mkbootimg
	./mkbootimg ideboot1.bin ideboot.com $@

#
# And the same for the NCR 5380.  A target on the 5380 reads back a
# volume laid out for the HD-DMA block for block, so this image is
# byte-compatible with the one above and differs only in which loader is
# in it.
#
NCRBOOTIMGS = ncrbootimg

ncrbootimgs: $(NCRBOOTIMGS)

ncrbootimg: ncrboot1.bin ncrboot.com mkbootimg
	./mkbootimg ncrboot1.bin ncrboot.com $@

# The floppy boot image: one side of cylinder 0, ten blocks.  Block 0 is
# djboot1.bin's first 128 bytes - level 0, which is all the firmware
# takes - block 1 is the rest of it - level 1, which reads blocks 2
# through 9 into STAGE and unpacks the second level from them.  The
# second level is the DJ boot, not mwboot.com: it is the floppy's, and
# mwboot.com speaks the HD-DMA.
#
# Splitting at 128 is the one thing that has to be right: level 1's
# labels are counted from 0080, so they only come out at 0100, where
# level 0's SETDMA puts its block, if level 0 is exactly what the
# firmware copies.  So it is checked - byte 128 must be level 1's first
# opcode, ld hl,BASE (21).
djload: djboot1.bin djboot.com
	@set -e; \
	b=`od -An -tx1 -j 128 -N 1 djboot1.bin | tr -d ' \n'`; \
	[ "$$b" = 21 ] || { echo "djload: byte 128 of djboot1.bin is $$b, not 21: level 0 is not 128 bytes"; exit 1; }
	dd if=/dev/zero of=djload bs=512 count=10
	dd if=djboot1.bin of=djload bs=1 count=128 conv=notrunc
	dd if=djboot1.bin of=djload bs=1 skip=128 seek=512 conv=notrunc
	dd if=djboot.com of=djload bs=512 seek=2 conv=notrunc

# The second level is linked manually, not by ccc, so its own crt0
# (sexit.s) replaces the library's.  ccc always links crt0.o, whose
# start() reads argc/argv off the stack - and the loader has none: it is
# entered by mwboot1 with an empty stack.  sexit.s provides start() and
# sexit(), so the library's exit and the stdio it flushes stay out,
# which is worth 618 bytes of text and 578 of bss.
#
# The objects are built with ccc -c and handed to mxld in order; sexit.o
# first so start() lands at the text offset mwboot1 jumps to.  That link
# order is not the one below, so the list is here for the dependency and
# the recipe spells the order it means.
OBJS = boot.o mwio.o sexit.o

mwboot.com: $(OBJS)
	$(MXLD) -s -o mwboot.com -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o mwio.o $(CCCLIB)/libc.a $(CCCLIB)/libu.a $(CCCLIB)/libc.a

#
# The floppy second level: the same boot.c with djio.o in place of
# mwio.o.  Both read a block, and boot.c does not know which it is
# talking to.
#
djboot.com: boot.o djio.o sexit.o
	$(MXLD) -s -o djboot.com -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o djio.o $(CCCLIB)/libc.a $(CCCLIB)/libu.a $(CCCLIB)/libc.a

#
# The IDE second level: the same boot.c with ideio.o in place of mwio.o.
# reset() and readblock() keep their names and their meaning across the
# three drivers, so nothing in boot.c knows which card it is.
#
ideboot.com: boot.o ideio.o sexit.o
	$(MXLD) -s -o ideboot.com -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ideio.o $(CCCLIB)/libc.a $(CCCLIB)/libu.a $(CCCLIB)/libc.a

#
# The NCR 5380 second level: the same boot.c with ncrio.o in place of
# mwio.o, for the reason the other two give - reset() and readblock()
# keep their names and their meaning across all four drivers.
#
ncrboot.com: boot.o ncrio.o sexit.o
	$(MXLD) -s -o ncrboot.com -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ncrio.o $(CCCLIB)/libc.a $(CCCLIB)/libu.a $(CCCLIB)/libc.a

boot.o: boot.c
	$(CCC) -c -i$(INC) -o boot.o boot.c
mwio.o: mwio.c
	$(CCC) -c -i$(INC) -o mwio.o mwio.c
djio.o: djio.c
	$(CCC) -c -i$(INC) -o djio.o djio.c
ideio.o: ideio.c
	$(CCC) -c -i$(INC) -o ideio.o ideio.c
ncrio.o: ncrio.c
	$(CCC) -c -i$(INC) -o ncrio.o ncrio.c
sexit.o: sexit.s
	$(CCC) -c -o sexit.o sexit.s

