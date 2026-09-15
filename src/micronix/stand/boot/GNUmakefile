#
# cross build for the two-level hard disk boot
#
# stand/boot/GNUmakefile
#
# The loader is standalone: it is entered by the rom with an empty
# stack, not by exec(), so it links its own crt0 (sexit.s provides
# start) and links ccc's own libc, not the tree's.  mwboot1.s is a
# sector, not a program - assembled and linked at 0x100, then the
# 16-byte object header is cut off.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= boot
CLASS	= stand

# mkbootimg.c is a host program and mwboot1.s/djboot1.s are sectors, so
# none of them belongs in the source lists the standard rules turn into
# objects.  boot.c is the filesystem code, shared by both boots; mwio.c
# and djio.c are the two device drivers, one per controller.
CSRCS	= boot.c mwio.c djio.c
ASSRCS	= sexit.s

# The main artifact, and the things built beside it.
TARGET	= mwboot.com
TARGETS	= mwboot1.bin bootimgs djboot.com djboot1.bin djload

# The loader links ccc's own libc, named by path below, so the tree's
# libc must not be a prerequisite of the link.
TREELIBS =

CCCTREE ?= $(HOME)/src/ccc
CCC	 = $(CCCTREE)/desthost/bin/ccc
MXLD	 = $(CCCTREE)/desthost/libexec/mxld

include $(DEPTH)/GNUmakefile.inc

#
# The second level.  Linked manually, not by ccc, so its own crt0
# (sexit.s) replaces the library's.  ccc always links crt0.o, whose
# start() reads argc/argv off the stack, and the loader is entered by
# mwboot1 with none.  sexit.o is first so start() lands at the text
# offset mwboot1 jumps to.
#
mwboot.com: boot.o mwio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCTREE)/desthost/lib \
	    sexit.o boot.o mwio.o \
	    $(CCCTREE)/desthost/lib/libc.a $(CCCTREE)/desthost/lib/libu.a \
	    $(CCCTREE)/desthost/lib/libc.a

#
# The floppy second level: the same boot.c, with the DJ-DMA driver
# (djio.c) in place of mwio.c.  reset() and readblock() have the same
# names in both drivers, so the filesystem code links against either
# unchanged.
#
djboot.com: boot.o djio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCTREE)/desthost/lib \
	    sexit.o boot.o djio.o \
	    $(CCCTREE)/desthost/lib/libc.a $(CCCTREE)/desthost/lib/libu.a \
	    $(CCCTREE)/desthost/lib/libc.a

#
# The first level is a sector: the rom reads 512 bytes to 0100 and
# jumps, so there is nowhere to put a header and nothing to strip one.
# The link is stripped and the 16-byte object header cut off.
#
mwboot1.bin: mwboot1.s
	$(CCC) $(CCFLAGS) -c -o mwboot1.o mwboot1.s
	$(MXLD) -s -Ttext=0x100 -o mwboot1.abs mwboot1.o
	dd if=mwboot1.abs of=$@ bs=16 skip=1

#
# The floppy first level, djboot1.s, built the same way.  It differs
# from mwboot1.s only in the read: the DJ-DMA runs a channel program,
# so djboot1 plants SETDMA/SREAD/HALT at the channel address and pulses
# 00ef instead of writing the HD-DMA command block.
#
djboot1.bin: djboot1.s
	$(CCC) $(CCFLAGS) -c -o djboot1.o djboot1.s
	$(MXLD) -s -Ttext=0x100 -o djboot1.abs djboot1.o
	dd if=djboot1.abs of=$@ bs=16 skip=1

#
# What mkfs -i writes: the first level in block 0, the second level
# from block 1.  The second half of that first block holds a struct
# dlabel - the geometry - so an image is per drive, one for each entry
# in the table below (the drives from specs[] in sys/mw.c):
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

# A host program that writes the per-drive image; it includes the
# target's dlabel.h by path so the structure it writes cannot drift.
mkbootimg: mkbootimg.c
	cc -o $@ mkbootimg.c

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
# bootimg with no drive named is the 5 meg, which is what the images
# in disks/hdinstall are.
#
bootimg: bootimg-m5
	cp bootimg-m5 bootimg

#
# The floppy boot image: djboot1.bin in sector 0, djboot.com in sectors
# 1 through 8.  The DJ-DMA first level reads sectors 1-8 to STAGE and
# unpacks them; the second level is the floppy second level above.
#
djload: djboot1.bin djboot.com
	dd if=/dev/zero of=$@ bs=512 count=9 2>/dev/null
	dd if=djboot1.bin of=$@ bs=512 conv=notrunc 2>/dev/null
	dd if=djboot.com of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

# The loader is not a program to install: install-hwsim reads the
# boot images out of this directory.  What it builds instead is
# cleaned up here - mwboot1.o/mwboot1.abs and djboot1.o/djboot1.abs are
# made inside the *.bin recipes, and mkbootimg is a host tool.
GENERATED = mwboot1.o mwboot1.abs djboot1.o djboot1.abs mkbootimg \
	bootimg bootimg-* djboot.com djload

install:

.PHONY: bootimgs
