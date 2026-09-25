#
# micronix native standalone
#
# micronix/stand/boot/Makefile
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mwboot.com mwboot1.bin bootimgs djboot1.bin djload \
	ideboot.com ideboot1.bin idebootimgs

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
	rm -f $(OBJS) ideio.o *.nam *.tok *.x
	rm -f ideboot.com
	rm -f mwboot1.abs djboot1.abs mwboot1.bin djboot1.bin
	rm -f ideboot1.abs ideboot1.bin mwboot.com djload mkbootimg
	rm -f $(BOOTIMGS) $(IDEBOOTIMGS) bootimg

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

# djboot1.s is the same sector, for the DJ-DMA floppy controller.  It
# reads the second level with a channel program instead of the HD-DMA
# command block; everything else is mwboot1.s.
djboot1.bin: djboot1.s
	$(ASZ) djboot1.s
	$(WSLD) -s -Ttext=0x100 -o djboot1.abs djboot1.o
	dd if=djboot1.abs of=djboot1.bin bs=16 skip=1

# ideboot1.s is the same sector again, for the IDE card.  It writes the
# drive's task file and moves the sector through the 8255 a word at a
# time instead of handing the HD-DMA a command block; everything else is
# mwboot1.s.
ideboot1.bin: ideboot1.s
	$(ASZ) ideboot1.s
	$(WSLD) -s -Ttext=0x100 -o ideboot1.abs ideboot1.o
	dd if=ideboot1.abs of=ideboot1.bin bs=16 skip=1

#
# what mkfs -i writes: the first level in block 0, the second level from
# block 1, which is where the first level looks for it.
#
# The second half of that first block is not the first level's: it holds
# a struct dlabel - the geometry, where the boot area starts, and the
# rotation mw.c applies.  So an image is per drive, and there is one for
# each entry in the table below.  mkfs writes the same structure when it
# installs a boot into a filesystem it is making, and overwrites this
# one; a boot put on a disk any other way keeps what is built in here.
#
# The drives, from specs[] in sys/mw.c, which is the table cmd/mkfs
# carries too - name:tracks:heads:sectors.  The name is the drive's, so
# bootimg-m5 goes with /dev/m5a.
#
#	153 4 17	10404	 5 meg
#	306 4 17	20808	10 meg
#	306 6 17	31212	16 meg
#	640 6 17	65280	32 meg
#	733 5 17	62305	40 meg
#
DRIVES = m5:153:4:17 m10:306:4:17 m16:306:6:17 m32:640:6:17 m40:733:5:17

BOOTIMGS = $(foreach d,$(DRIVES),bootimg-$(word 1,$(subst :, ,$(d))))

bootimgs: $(BOOTIMGS)

#
# A host program, and it includes the target's sys/dlabel.h so the
# structure it writes cannot drift from the one mkfs writes.
#
mkbootimg: mkbootimg.c
	cc -o mkbootimg mkbootimg.c

$(BOOTIMGS): mwboot1.bin mwboot.com mkbootimg
	@set -e; for d in $(DRIVES); do \
		n=`echo $$d | cut -d: -f1`; \
		[ bootimg-$$n = $@ ] || continue; \
		./mkbootimg mwboot1.bin mwboot.com \
		    `echo $$d | cut -d: -f2` \
		    `echo $$d | cut -d: -f3` \
		    `echo $$d | cut -d: -f4` $@; \
		echo "$@: `echo $$d | cut -d: -f2`/`echo $$d | cut -d: -f3`/`echo $$d | cut -d: -f4`"; \
	done

#
# bootimg with no drive named is the 5 meg, which is what the images in
# disks/hdinstall are.
#
bootimg: bootimg-m5
	cp bootimg-m5 bootimg

#
# The same table again for the IDE card, and a second set of images: what
# goes in these is ideboot1/ideboot, which drive the 8255, and an image
# is a boot for one card.  They are per drive for the reason above - the
# label carries the geometry and this loader reads it.
#
IDEBOOTIMGS = $(foreach d,$(DRIVES),idebootimg-$(word 1,$(subst :, ,$(d))))

idebootimgs: $(IDEBOOTIMGS)

$(IDEBOOTIMGS): ideboot1.bin ideboot.com mkbootimg
	@set -e; for d in $(DRIVES); do \
		n=`echo $$d | cut -d: -f1`; \
		[ idebootimg-$$n = $@ ] || continue; \
		./mkbootimg ideboot1.bin ideboot.com \
		    `echo $$d | cut -d: -f2` \
		    `echo $$d | cut -d: -f3` \
		    `echo $$d | cut -d: -f4` $@; \
		echo "$@: `echo $$d | cut -d: -f2`/`echo $$d | cut -d: -f3`/`echo $$d | cut -d: -f4`"; \
	done

# The floppy boot image: djboot1.bin in sector 0, mwboot.com in sectors
# 1 through 8, which is what djboot1.s reads.
djload: djboot1.bin mwboot.com
	dd if=/dev/zero of=djload bs=512 count=9
	dd if=djboot1.bin of=djload bs=512 conv=notrunc
	dd if=mwboot.com of=djload bs=512 seek=1 conv=notrunc

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
# The IDE second level: the same boot.c with ideio.o in place of mwio.o.
# reset() and readblock() keep their names and their meaning across the
# three drivers, so nothing in boot.c knows which card it is.
#
ideboot.com: boot.o ideio.o sexit.o
	$(MXLD) -s -o ideboot.com -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ideio.o $(CCCLIB)/libc.a $(CCCLIB)/libu.a $(CCCLIB)/libc.a

boot.o: boot.c
	$(CCC) -c -i$(INC) -o boot.o boot.c
mwio.o: mwio.c
	$(CCC) -c -i$(INC) -o mwio.o mwio.c
ideio.o: ideio.c
	$(CCC) -c -i$(INC) -o ideio.o ideio.c
sexit.o: sexit.s
	$(CCC) -c -o sexit.o sexit.s

