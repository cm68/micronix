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
CSRCS	= boot.c mwio.c djio.c ideio.c ncrio.c hdcaio.c
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
EXTRA	= mkbootimg.c mwboot1.s djboot1.s ideboot1.s ncrboot1.s hdcaboot1.s README

# The main artifact, and the things built beside it.
TARGET	= mwboot.com
TARGETS	= mwboot1.bin bootimgs djboot.com djboot1.bin djbootimgs \
	  ideboot.com ideboot1.bin idebootimgs \
	  ncrboot.com ncrboot1.bin ncrbootimgs \
	  hdcaboot.com hdcaboot1.bin hdcabootimgs

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
# Level 3.  Linked manually, not by ccc, so its own crt0
# (sexit.s) replaces the library's.  ccc always links crt0.o, whose
# start() reads argc/argv off the stack, and the loader is entered by
# mwboot1 with none.  sexit.o is first so start() lands at the text
# offset mwboot1 jumps to.
#
mwboot.com: boot.o mwio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o mwio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libunix.a \
	    $(CCCLIB)/libc.a

#
# The floppy: the same boot.c, with the DJ-DMA driver
# (djio.c) in place of mwio.c.  reset() and readblock() have the same
# names in both drivers, so the filesystem code links against either
# unchanged.
#
djboot.com: boot.o djio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o djio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libunix.a \
	    $(CCCLIB)/libc.a

#
# The IDE: the same boot.c again, with the driver for the
# S100Computers MYIDE board (ideio.c) in place of mwio.c.  None of boot.c
# changes and none of it needs to know which card it is talking to.
#
ideboot.com: boot.o ideio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ideio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libunix.a \
	    $(CCCLIB)/libc.a

#
# The NCR 5380: the same boot.c again, with the driver for the
# NCR 5380 host adapter (ncrio.c) in place of mwio.c.  None of boot.c
# changes and none of it needs to know which card it is talking to.
#
ncrboot.com: boot.o ncrio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o ncrio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libunix.a \
	    $(CCCLIB)/libc.a

#
# The HDCA: the same boot.c again, with the driver for the
# port-mapped HDCA (hdcaio.c) in place of mwio.c.  reset() and
# readblock() keep their names and their meaning across all five
# drivers, so nothing in boot.c knows which card it is.
#
hdcaboot.com: boot.o hdcaio.o sexit.o
	$(MXLD) -s -o $@ -Ttext=0x100 -L$(CCCLIB) \
	    sexit.o boot.o hdcaio.o \
	    $(CCCLIB)/libc.a $(CCCLIB)/libunix.a \
	    $(CCCLIB)/libc.a

#
# Level 2 is a sector: the rom reads 512 bytes to 0100 and
# jumps, so there is nowhere to put a header and nothing to strip one.
# The link is stripped and the 16-byte object header cut off.
#
mwboot1.bin: mwboot1.s
	$(CCC) $(CCFLAGS) -c -o mwboot1.o mwboot1.s
	$(MXLD) -s -Ttext=0x100 -o mwboot1.abs mwboot1.o
	dd if=mwboot1.abs of=$@ bs=16 skip=1

#
# The floppy's level 2 sector, djboot1.s, built like the other four: it
# links at 0100 because that is where the rom puts a block it read, and
# it must fit in the half of the sector ahead of the label - mkbootimg
# refuses one that does not.  It used to lead with a 128 byte level 0,
# because the card's own bootstrap copied only that much to 0080; that
# is switched off at the card (J2 A to B), so this is one sector like
# the others and the label is in the half behind it.
#
djboot1.bin: djboot1.s
	$(CCC) $(CCFLAGS) -c -o djboot1.o djboot1.s
	$(MXLD) -s -Ttext=0x100 -o djboot1.abs djboot1.o
	dd if=djboot1.abs of=$@ bs=16 skip=1

#
# The IDE's level 2 sector, ideboot1.s, built the same way.  It differs from
# mwboot1.s in the read as well: the HD-DMA is told where the sector goes
# by a command block in memory, and the IDE card has no such block, so
# this one writes the drive's task file and moves the sector through the
# 8255 port by port.  It also has to be shorter than the other two - the
# label sits at byte 256 of the sector and mkbootimg refuses an image
# whose level 2 runs into it.
#
ideboot1.bin: ideboot1.s
	$(CCC) $(CCFLAGS) -c -o ideboot1.o ideboot1.s
	$(MXLD) -s -Ttext=0x100 -o ideboot1.abs ideboot1.o
	dd if=ideboot1.abs of=$@ bs=16 skip=1

#
# The NCR 5380's level 2 sector, ncrboot1.s, built the same way.  It differs from
# ideboot1.s in the read as well: the IDE card's registers are behind an
# 8255, and the 5380 is driven one byte at a time through a REQ/ACK
# handshake, so every sector here is its own command - select the target,
# send a READ(10), take the data, take the status and message.  It has the
# same 256-byte budget: the label sits at byte 256 of the sector and
# mkbootimg refuses an image whose level 2 runs into it.
#
ncrboot1.bin: ncrboot1.s
	$(CCC) $(CCFLAGS) -c -o ncrboot1.o ncrboot1.s
	$(MXLD) -s -Ttext=0x100 -o ncrboot1.abs ncrboot1.o
	dd if=ncrboot1.abs of=$@ bs=16 skip=1

#
# The HDCA's level 2 sector, hdcaboot1.s, built the same way.  It differs from
# ideboot1.s in the read again: the HDCA has no 8255 and no task file,
# so this one writes its own header bytes - head, track, sector, key -
# through the data port and pulls the sector out a byte at a time.  It
# has the same 256-byte budget: the label sits at byte 256 of the sector
# and mkbootimg refuses a level 2 that runs into it.
#
hdcaboot1.bin: hdcaboot1.s
	$(CCC) $(CCFLAGS) -c -o hdcaboot1.o hdcaboot1.s
	$(MXLD) -s -Ttext=0x100 -o hdcaboot1.abs hdcaboot1.o
	dd if=hdcaboot1.abs of=$@ bs=16 skip=1

#
# What mkfs -i writes: level 2 in block 0, level 3
# from block 1.  One image, and not one per drive: the second half of
# that first block is a struct dlabel, and on a hard disk the label is
# the drive's.  mkfs -i takes the shape from the device node it is
# handed rather than from the boot file, and writes the label into the
# block it puts the boot in - putlabel in cmd/mkfs/mkfsfunc.c, the same
# code that stamps a boot installed any other way.  So the half is left
# empty here for that writer to fill, and this tool is never told a
# geometry it would only be guessing at.  See stand/boot/mkbootimg.c.
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

# A host program that writes the image; it includes the target's
# dlabel.h by path so the structure it writes cannot drift.
mkbootimg: mkbootimg.c
	cc -o $@ mkbootimg.c

bootimg: mwboot1.bin mwboot.com mkbootimg
	./mkbootimg mwboot1.bin mwboot.com $@

#
# And the same for the IDE card: one image, for the same reason, and a
# separate file from bootimg rather than the same one built twice,
# because what goes in them differs.  bootimg holds mwboot1/mwboot,
# which speak the HD-DMA's command block; this holds ideboot1/ideboot,
# which write the drive's task file and move the sector through the
# 8255.  An image is a boot for one card and cannot be both.
#
IDEBOOTIMGS = idebootimg

idebootimgs: $(IDEBOOTIMGS)

idebootimg: ideboot1.bin ideboot.com mkbootimg
	./mkbootimg ideboot1.bin ideboot.com $@

#
# And the same for the NCR 5380 card.  Its target 0 speaks to a volume
# laid out for the HD-DMA block for block, so this image is
# byte-compatible with the one above and differs only in which loader is
# in it.
#
NCRBOOTIMGS = ncrbootimg

ncrbootimgs: $(NCRBOOTIMGS)

ncrbootimg: ncrboot1.bin ncrboot.com mkbootimg
	./mkbootimg ncrboot1.bin ncrboot.com $@

#
# And the same for the HDCA.  Its target 0 speaks to a volume laid out
# for the HD-DMA block for block, so this image is byte-compatible with
# the one above and differs only in which loader is in it.
#
HDCABOOTIMGS = hdcabootimg

hdcabootimgs: $(HDCABOOTIMGS)

hdcabootimg: hdcaboot1.bin hdcaboot.com mkbootimg
	./mkbootimg -a 0x100 hdcaboot1.bin hdcaboot.com $@

#
# The floppy boot images: one per medium, because the label is what
# carries the geometry and the medium is what the label describes.  Block
# 0 holds djboot1.bin - the level 2 sector, in the half of the block ahead
# of the label - and the label is at DL_OFFSET; the rest of cylinder 0 is
# empty, and level 3, djboot.com, begins at the first sector of cylinder
# 1, which level 2 reads into STAGE and unpacks.  One rule for every
# medium: an eight inch diskette cannot put level 3 at block 1, because
# its cylinder 0 is 26 sectors of 128 bytes and level 3 is four kilobytes
# of it.  See mkbootimg.c and djboot1.s.  What -d adds is the label, and
# a diskette's image carries one because there is no installer to write
# it onto the drive the image is later laid on.
#
# mkbootimg refuses a level 2 that runs into the label, and a level 3
# that reaches 0ff0 - the two things that could be wrong with an image,
# both of which would otherwise be boots that go quiet.  Nothing else
# measures either one.
#
# The media, as cmd/mkfs/mkfs.h's table has them: name:cylinders:heads:
# blocks:firstsec:toff.  Four, and not the 5 1/4 single sided 40/1/10 -
# 194 K cannot hold a running system, so it is not a medium this builds
# for.
#
# The fourth field is blocks per track, in 512 byte units, not physical
# sectors: a 1K medium's five sectors read as ten.  d5soft is the soft
# sectored 5 1/4 inch medium, five 1024 byte sectors to a track, and
# d5ds is the hard sectored one, ten 512 byte sectors - both ten blocks.
#
# firstsec is where the medium numbers its sectors from, and it is one
# for the soft sectored media and zero for the hard sectored five inch
# one.  It is not decoration: level 2 reads it out of the label to know
# which sector to ask for first, and every reader finds the label by
# looking for its magic in the medium's first sector, which it can only
# do if it guesses the numbering right.  See sys/dlabel.h and mkbootimg.c.
#
# How many blocks a sector is has no field of its own: it is djio.c's rule
# over the other two - a soft sectored medium with ten or sixteen blocks
# to a track is 1024 bytes, and every other medium here is 512 - and both
# level 2 and level 3 work it out from the label for themselves.
#
FLOPPIES = d8ss:77:1:15:1:2 d8ds:77:2:15:1:2 d5ds:40:2:10:0:2 \
	d5soft:40:2:10:1:2
DJBOOTIMGS = $(foreach f,$(FLOPPIES),djbootimg-$(word 1,$(subst :, ,$(f))))

djbootimgs: $(DJBOOTIMGS)

$(DJBOOTIMGS): djboot1.bin djboot.com mkbootimg
	@set -e; \
	for d in $(FLOPPIES); do \
		n=`echo $$d | cut -d: -f1`; \
		[ djbootimg-$$n = $@ ] || continue; \
		./mkbootimg -d djboot1.bin djboot.com \
		    `echo $$d | cut -d: -f2` \
		    `echo $$d | cut -d: -f3` \
		    `echo $$d | cut -d: -f4` \
		    `echo $$d | cut -d: -f5` \
		    `echo $$d | cut -d: -f6` $@; \
		v=`od -An -tx1 -j 260 -N 2 $@ | tr -d ' \n'`; \
		[ "$$v" = 0300 ] || { echo "$@: label version is $$v, not 3: not the slice idiom"; exit 1; }; \
		f=`echo $$d | cut -d: -f5`; \
		v=`od -An -tx1 -j 268 -N 1 $@ | tr -d ' \n'`; \
		[ "$$v" = "0$$f" ] || { echo "$@: the label's first sector is $$v, not 0$$f"; exit 1; }; \
		echo "$@: `echo $$d | cut -d: -f2`/`echo $$d | cut -d: -f3`/`echo $$d | cut -d: -f4` first $$f toff `echo $$d | cut -d: -f6`"; \
	done

# The loader is not a program to install: install-hwsim reads the
# boot images out of this directory.  What it builds instead is
# cleaned up here - mwboot1.o/mwboot1.abs and djboot1.o/djboot1.abs are
# made inside the *.bin recipes, and mkbootimg is a host tool.  The
# bootimg-m* names are the per-drive hard disk images this used to
# build, before the geometry moved wholly to mkfs; a tree that predates
# that still has them, so they are cleared by pattern rather than by
# name.
GENERATED = mwboot1.o mwboot1.abs djboot1.o djboot1.abs \
	ideboot1.o ideboot1.abs ncrboot1.o ncrboot1.abs \
	hdcaboot1.o hdcaboot1.abs mkbootimg \
	bootimg bootimg-* idebootimg idebootimg-* ncrbootimg ncrbootimg-* \
	hdcabootimg hdcabootimg-* \
	djboot.com djbootimg-*

install:

.PHONY: bootimgs djbootimgs idebootimgs ncrbootimgs hdcabootimgs
