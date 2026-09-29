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

# mkbootimg.c is a host program and mwboot1.s/djboot1.s/ideboot1.s/
# ncrboot1.s are sectors, so none of them belongs in the source lists the
# standard rules turn into objects.  boot.c is the filesystem code, shared
# by all four boots; mwio.c, djio.c, ideio.c and ncrio.c are the device
# drivers, one per controller - HD-DMA, DJ-DMA, the IDE card and the NCR
# 5380 SCSI card.
CSRCS	= boot.c mwio.c djio.c ideio.c ncrio.c
ASSRCS	= sexit.s

#
# That exclusion is about the build and not about the tar.  The source
# tar carries $(SRCS), which is the build lists plus $(EXTRA), so a file
# that is a source but not a build input lists itself there.  Without
# this the installed /usr/src/stand cannot build at all: the Makefile
# beside this one opens with mwboot1.bin from mwboot1.s, and neither it
# nor mkbootimg.c was in the tar - nor were the other three first
# levels, nor the README that describes what they do.
#
EXTRA	= mkbootimg.c mwboot1.s djboot1.s ideboot1.s ncrboot1.s README

# The main artifact, and the things built beside it.
TARGET	= mwboot.com
TARGETS	= mwboot1.bin bootimgs djboot.com djboot1.bin djbootimgs \
	  ideboot.com ideboot1.bin idebootimgs \
	  ncrboot.com ncrboot1.bin ncrbootimgs

# The loader links ccc's own libc, named by path below, so the tree's
# libc must not be a prerequisite of the link.
TREELIBS =

# The compiler and linker are this tree's own.  CCC defaults to
# $(BIN)/mxccc in GNUmakefile.inc; the linker and the runtime libc it
# links against live beside it, the latter under $(HOSTROOT)/ccc/lib
# where hostrt puts it.
MXLD	 = $(BIN)/mxld
CCCLIB	 = $(HOSTROOT)/ccc/lib

include $(DEPTH)/GNUmakefile.inc

#
# The second level.  Linked manually, not by ccc, so its own crt0
# (sexit.s) replaces the library's.  ccc always links crt0.o, whose
# start() reads argc/argv off the stack, and the loader is entered by
# mwboot1 with none.  sexit.o is first so start() lands at the text
# offset mwboot1 jumps to.
#
mwboot.com: boot.o mwio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o mwio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libu.a \
	    $(CCCLIB)/libc.a

#
# The floppy second level: the same boot.c, with the DJ-DMA driver
# (djio.c) in place of mwio.c.  reset() and readblock() have the same
# names in both drivers, so the filesystem code links against either
# unchanged.
#
djboot.com: boot.o djio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o djio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libu.a \
	    $(CCCLIB)/libc.a

#
# The IDE second level: the same boot.c again, with the driver for the
# S100Computers MYIDE board (ideio.c) in place of mwio.c.  None of boot.c
# changes and none of it needs to know which card it is talking to.
#
ideboot.com: boot.o ideio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ideio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libu.a \
	    $(CCCLIB)/libc.a

#
# The NCR 5380 second level: the same boot.c again, with the driver for the
# NCR 5380 host adapter (ncrio.c) in place of mwio.c.  None of boot.c
# changes and none of it needs to know which card it is talking to.
#
ncrboot.com: boot.o ncrio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ncrio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libu.a \
	    $(CCCLIB)/libc.a

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
# The floppy first level, djboot1.s, built differently, because on this
# controller the first level is two levels.  The firmware copies only
# 128 bytes to 0080, so the file leads with a 128-byte level 0 whose
# whole job is to read the next block and jump into it; the level 1
# behind it is mwboot1.s with a channel program for a read, and it links
# at 0100 because that is where level 0 puts it.  So the link base is
# 0080, not 0100, and the 128 bytes between are the pad the head does
# not use - which is also why level 1's labels need the padding to be
# exact (see the djload rule).
#
djboot1.bin: djboot1.s
	$(CCC) $(CCFLAGS) -c -o djboot1.o djboot1.s
	$(MXLD) -s -Ttext=0x80 -o djboot1.abs djboot1.o
	dd if=djboot1.abs of=$@ bs=16 skip=1

#
# The IDE first level, ideboot1.s, built the same way.  It differs from
# mwboot1.s in the read as well: the HD-DMA is told where the sector goes
# by a command block in memory, and the IDE card has no such block, so
# this one writes the drive's task file and moves the sector through the
# 8255 port by port.  It also has to be shorter than the other two - the
# label sits at byte 256 of the sector and mkbootimg refuses an image
# whose first level runs into it.
#
ideboot1.bin: ideboot1.s
	$(CCC) $(CCFLAGS) -c -o ideboot1.o ideboot1.s
	$(MXLD) -s -Ttext=0x100 -o ideboot1.abs ideboot1.o
	dd if=ideboot1.abs of=$@ bs=16 skip=1

#
# The NCR 5380 first level, ncrboot1.s, built the same way.  It differs from
# ideboot1.s in the read as well: the IDE card's registers are behind an
# 8255, and the 5380 is driven one byte at a time through a REQ/ACK
# handshake, so every sector here is its own command - select the target,
# send a READ(10), take the data, take the status and message.  It has the
# same 256-byte budget: the label sits at byte 256 of the sector and
# mkbootimg refuses an image whose first level runs into it.
#
ncrboot1.bin: ncrboot1.s
	$(CCC) $(CCFLAGS) -c -o ncrboot1.o ncrboot1.s
	$(MXLD) -s -Ttext=0x100 -o ncrboot1.abs ncrboot1.o
	dd if=ncrboot1.abs of=$@ bs=16 skip=1

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
# And the same for the IDE card.  One image per drive for the same
# reason: the label is what carries the geometry, this loader reads it,
# and a card that addresses by LBA still has to be told where a
# filesystem block is on the platter.
#
# These are separate files from the bootimg-* above rather than the same
# ones built twice, because what goes in them differs: bootimg-* holds
# mwboot1/mwboot, which speak HD-DMA, and these hold ideboot1/ideboot,
# which speak the 8255.  An image is a boot for one card and cannot be
# both.
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

#
# And the same for the NCR 5380 card.  The geometry argument is the same
# DRIVES table for the same reason: the label is what carries it, this
# loader reads it, and a card that addresses by LBA still has to be told
# where a filesystem block is on the platter.  The NCR 5380 card's own target
# 0 speaks to a volume laid out for the HD-DMA block for block, so the
# images are byte-compatible with the ones above and differ only in which
# loader is in them.
#
NCRBOOTIMGS = $(foreach d,$(DRIVES),ncrbootimg-$(word 1,$(subst :, ,$(d))))

ncrbootimgs: $(NCRBOOTIMGS)

$(NCRBOOTIMGS): ncrboot1.bin ncrboot.com mkbootimg
	@set -e; for d in $(DRIVES); do \
		n=`echo $$d | cut -d: -f1`; \
		[ ncrbootimg-$$n = $@ ] || continue; \
		./mkbootimg ncrboot1.bin ncrboot.com \
		    `echo $$d | cut -d: -f2` \
		    `echo $$d | cut -d: -f3` \
		    `echo $$d | cut -d: -f4` $@; \
		echo "$@: `echo $$d | cut -d: -f2`/`echo $$d | cut -d: -f3`/`echo $$d | cut -d: -f4`"; \
	done

#
# The floppy boot images: one per medium, because the label is what
# carries the geometry and the medium is what the label describes.  Block
# 0 is djboot1.bin's first 128 bytes - level 0, which is all the firmware
# takes - then the label at DL_OFFSET, block 1 is the rest of djboot1.bin
# - level 1, which reads blocks 2 through 9 into STAGE and unpacks the
# second level from them.
#
# Splitting at 128 is the one thing that has to be right: level 1's
# labels are the linker's, counted from 0080, so they only come out at
# 0100 - where level 0's SETDMA puts that block - if level 0 is exactly
# what the firmware copies.  Get it wrong and level 1 runs at the wrong
# address, and nothing about the image would look wrong.  So it is
# checked: byte 128 must be level 1's first opcode, ld hl,BASE (21 00 01).
# The split itself is mkbootimg's (-d), not a pair of dd lines, so the
# same boundary the check tests is the one the image is built with.
#
# The media, as cmd/mkfs/mkfs.h's table has them: name:cylinders:heads:
# sectors:firstsec:toff.  Three, and not the 5 1/4 single sided 40/1/10 -
# 194 K cannot hold a running system, so it is not a medium this builds
# for.
#
# firstsec is where the medium numbers its sectors from, and it is one
# for the soft sectored eight inch media and zero for the hard sectored
# five inch one.  It is not decoration: the loader reads the label by
# finding its magic in the first sector of track 0, which it can only do
# if it guesses the numbering right, and it then uses this field for
# every block after that.  Level 0 cannot wait for the label - it has to
# name a sector to read the label's own block - so mkbootimg writes the
# same number into level 0's last two bytes as well, and both places are
# checked below.  See sys/dlabel.h, djboot1.s's FSEC and mkbootimg.c.
#
# The last field is the medium's format byte, in hex, and only the hard
# sectored five inch medium has one.  The driver identifies that medium by
# looking the byte up in its table of formats, and a byte of zero means no
# lookup at all, which leaves the driver on the default row for a five inch
# drive: 35 cylinders, one head, ten sectors per cylinder, 330 blocks - the
# filesystem still mounts and everything past block 330 reads as nothing,
# which is where the kernel's own modules are.  0xD0 is 40 cylinders, two
# heads, 10 sectors, 512 bytes, which is what d5ds is.  The eight inch
# media are soft sectored and the driver never looks, so they carry zero.
# mkbootimg writes the byte inside level 0's 128 bytes, which is the only
# part of sector 0 the driver can read it out of; it is checked below.
#
FLOPPIES = d8ss:77:1:15:1:2:00 d8ds:77:2:15:1:2:00 d5ds:40:2:10:0:2:d0
DJBOOTIMGS = $(foreach f,$(FLOPPIES),djbootimg-$(word 1,$(subst :, ,$(f))))

djbootimgs: $(DJBOOTIMGS)

$(DJBOOTIMGS): djboot1.bin djboot.com mkbootimg
	@set -e; \
	b=`od -An -tx1 -j 128 -N 1 djboot1.bin | tr -d ' \n'`; \
	[ "$$b" = 21 ] || { echo "$@: byte 128 of djboot1.bin is $$b, not 21: level 0 is not 128 bytes"; exit 1; }; \
	for d in $(FLOPPIES); do \
		n=`echo $$d | cut -d: -f1`; \
		[ djbootimg-$$n = $@ ] || continue; \
		./mkbootimg -d djboot1.bin djboot.com \
		    `echo $$d | cut -d: -f2` \
		    `echo $$d | cut -d: -f3` \
		    `echo $$d | cut -d: -f4` \
		    `echo $$d | cut -d: -f5` \
		    `echo $$d | cut -d: -f6` \
		    0x`echo $$d | cut -d: -f7` $@; \
		v=`od -An -tx1 -j 260 -N 2 $@ | tr -d ' \n'`; \
		[ "$$v" = 0200 ] || { echo "$@: label version is $$v, not 2: not the slice idiom"; exit 1; }; \
		f=`echo $$d | cut -d: -f5`; \
		v=`od -An -tx1 -j 314 -N 1 $@ | tr -d ' \n'`; \
		[ "$$v" = "0$$f" ] || { echo "$@: the label's first sector is $$v, not 0$$f"; exit 1; }; \
		v=`od -An -tx1 -j 127 -N 1 $@ | tr -d ' \n'`; \
		[ "$$v" = a5 ] || { echo "$@: level 0's marker is $$v, not a5: level 0 is not 128 bytes"; exit 1; }; \
		v=`od -An -tx1 -j 126 -N 1 $@ | tr -d ' \n'`; \
		[ "$$v" = "0$$f" ] || { echo "$@: level 0 asks for sector $$v first, not 0$$f"; exit 1; }; \
		c=`echo $$d | cut -d: -f7`; \
		v=`od -An -tx1 -j 92 -N 1 $@ | tr -d ' \n'`; \
		[ "$$v" = "$$c" ] || { echo "$@: the format byte is $$v, not $$c"; exit 1; }; \
		echo "$@: `echo $$d | cut -d: -f2`/`echo $$d | cut -d: -f3`/`echo $$d | cut -d: -f4` first $$f toff `echo $$d | cut -d: -f6`"; \
	done

# The loader is not a program to install: install-hwsim reads the
# boot images out of this directory.  What it builds instead is
# cleaned up here - mwboot1.o/mwboot1.abs and djboot1.o/djboot1.abs are
# made inside the *.bin recipes, and mkbootimg is a host tool.
GENERATED = mwboot1.o mwboot1.abs djboot1.o djboot1.abs \
	ideboot1.o ideboot1.abs ncrboot1.o ncrboot1.abs mkbootimg \
	bootimg bootimg-* djboot.com djbootimg-* idebootimg-* ncrbootimg-*

install:

.PHONY: bootimgs djbootimgs idebootimgs ncrbootimgs
