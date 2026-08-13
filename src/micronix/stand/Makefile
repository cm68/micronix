#
# micronix native standalone
#
# micronix/stand/Makefile
#
# Changed: <2023-07-10 14:38:40 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	mwboot.com mwboot1.bin bootimgs

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
INC = /usr/local/lib/include

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

# sexit.s answers crt0's "jp sexit" so that the library's exit - and the
# stdio it flushes - stays out.  It is worth 618 bytes of text and 578 of
# bss, which is the difference between the loader fitting under the
# kernel's load address and running into it.
mwboot.com: boot.c mwio.c sexit.s
	$(CCC) -S -i$(INC) -o mwboot.com boot.c mwio.c sexit.s

#
# Morrow's hard disc formatter, version 1.7 with the 1.8 proto drive
# table, ported off the Whitesmiths run time library - the WSLIB ifdefs
# in formatmw.c select the original calls back if that library returns.
#
# A CP/M program, not a micronix one, and it cannot become one: it hands
# the controller physical dma addresses, writes the channel address to
# physical 0x50, and issues out instructions.  None of that survives the
# MPZ80's map.  It runs under CP/M on the d1 simulation, which is where
# the format command's sector id table can be read off the wire instead
# of guessed at.
#
# Deliberately not in all: c1 dies on this file today, so a default
# build would stop here.  See ~/src/ccc/ARRAY_ERROR.
#
CCC = ccc

formatmw.com: formatmw.c
	$(CCC) -m cpm -o formatmw.com formatmw.c
