#
# cross build for the kernel
#
# sys/GNUmakefile
#
# "all" builds the kernel: unix, at a fixed text base of 0x1000.  ccc
# has no -tb, no overlay -r and no -x, so the link is done by calling
# mxld directly with -Ttext=0x1000.  The native Makefile beside this one
# does that same flat link, with the same linker source built for that
# host - it is plain "ld" over there, where it IS the system's linker.
# The Whitesmith's overlay link they both replaced (low.o/io.o/high.o
# through "link -tb0x1000 -r") is gone.
#
# update, clean and clobber are real.  update used to be
#
#	update:
#		(cd .. ; make update)
#
# which walked UP the tree - harmless only because the top level's
# update did nothing at the time.  Now that the top level recurses
# into sys, that would have been an infinite loop.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ..
DIR	= sys
CLASS	=
SRCDIR	= /usr/src/sys

TARGET	= unix

# the kernel includes its own headers, not the C library's.  -i, as
# the native Makefile beside this has always done.
INC	= -i$(DEPTH)/include

# build.h is written by the rule below; GNUmakefile.inc's clobber takes
# GENERATED away, so it is not left lying in the directory for the next
# build to include.
GENERATED = build.h

# The compiler and linker are this tree's own.  CCC defaults to
# $(BIN)/mxccc in GNUmakefile.inc; the linker mxld is beside it.
MXLD     = $(BIN)/mxld

# the kernel's objects, in link order: the entry header (uhdr.s) first
# so its jump lands at 0x1000.  inout.s is here because memdev.c,
# multio.c, mw.c, dj.c and cus.c all call in()/out(), though the old
# Whitesmiths-era makefile never listed it.  makedate.c is left out: it
# is not kernel source.  It is a micronix program - the native build
# compiles it with ccc and runs it under the simulator to write build.h -
# and the one thing this host cannot do with it is run it, which is what
# the build.h rule below works around.
# Leaf code that lives in the u page (see user.c).  It is assembled as
# .data and linked into upage.o, not into the main image.
# csv/q*/ldiv/tramp/sw*/oarg are the libccc runtime, copied from
# src/libc and re-sectioned to psect data.  They sat in leaf/ until that
# directory was flattened; the generic %.o:%.s rule assembles them here,
# with the rest of the kernel's sources.
LEAF_LIBC = csv.o idiv.o imul.o qadd.o qsub.o \
	qand.o qor.o qcmp.o qshl.o qsar.o qshr.o \
	qldst.o tramp.o swdisp.o swdispw.o oarg.o
LEAF_AS   = mem.o inout.o sub8.o unixtobcd.o $(LEAF_LIBC)
# Read-only kernel tables (syssw, biosw, ciosw) that also live in the u page.
LEAF_C    = consts.o rtc.o

# rtc.c is pure code (no static data), so re-section its single .text
# block to .data and park it in the u page, like the libccc runtime.
# ccc compiles it to rtc.rel as .text, and the -r -Sdata link folds every
# section of that into .data - the same fold highmem.o uses for the
# init-only code, where the old recipe re-sectioned the listing with sed.
# The explicit rule beats the %.o:%.c rule below, which would leave the
# object as .text and let the loader's data pass write it over the u page.
rtc.o: rtc.c
	$(CCC) $(CCFLAGS) $(INC) -c -o rtc.rel $<
	$(MXLD) -r -Sdata -o rtc.o rtc.rel
	rm -f rtc.rel

# consts.c needs the same fold for the same reason, one literal at a
# time: ccc puts the device-name strings the tables point at in .text,
# and the tables themselves are in the u page.  consts.o is where that
# showed what it costs - the final link pads text in upage.o out to
# the u page, tens of kilobytes of zeros in the middle of the image,
# which is past the blocks the boot loader reads and cost the machine
# its boot.
consts.o: consts.c
	$(CCC) $(CCFLAGS) $(INC) -c -o consts.rel $<
	$(MXLD) -r -Sdata -o consts.o consts.rel
	rm -f consts.rel

KERNEL_AS = uhdr.o intrpt.o mio.o textpad.o highmem.o
KERNEL_C  = mount.o balloc.o cio.o create.o exec.o fio.o fork.o \
	ialloc.o iget.o imap.o malloc.o name.o queue.o \
	reg.o sig.o sleep.o subc.o swap.o sys1.o system.o time.o \
	trap.o tty.o uio.o dj.o multio.o dlabel.o mw.o ide.o main.o
WIN_C     = open.o sys2.o rdwri.o memdev.o link.o access.o
HIGHMEM   = main_init.o cus.o inits.o
OBJS     = $(KERNEL_AS) $(LEAF_AS) $(LEAF_C) user.o upage.o $(KERNEL_C) $(WIN_C) $(HIGHMEM) low.o win.o
TREELIBS =

include $(DEPTH)/GNUmakefile.inc

#
# The buffer pool.  Unclamped by default: expand_bufs() mints headers up
# to BUFWIN (sys.h, the base of the page the buffers are reached
# through) and the pool is whatever fits between _ebss and there.  It is
# that page and not _upage above it because bwin() holds a segment in it
# for the whole of a buffer access.  The 0xd000 page used to be reserved
# for the superblock's window; the superblock is plain memory now (getsb,
# in uio.c) and mem.s puts back the page it borrows for user memory, so
# the headers carry on through it.  A cache bug that only shows itself
# with the full pool costs an hour of recompiling to find, so the clamp
# is still here, as an opt-in:
#
#	make DEBUGBUFS=1	32 buffers instead of the full budget
#
# It is a switch and not a value because the budget is not a tunable:
# the pool is sized by where .bss ends, which moves as the kernel grows,
# and MAXBUFS only ever shortens it.
#
# Appended to CCFLAGS here, after the include that sets it, so the
# default stays in one place.
#
ifdef DEBUGBUFS
CCFLAGS += -DDEBUGBUFS
endif

#
# Every object: the ones "all" links into unix, and the ones folded into
# one of those first (the u-page leaf code into upage.o, the init-only
# code into highmem.o).
#
objects: $(OBJS)

#
# main_init.c includes it, and the native build generates it with
# makedate - a Micronix program that wants types.h, std.h and a struct
# time this host has not got.  All the header carries is the date, so
# the cross build writes its own rather than fail main_init.c for a
# reason that has nothing to do with the compiler.
#
# Phony so the date is written afresh on every build: build.h has no
# prerequisite to be newer than, so a real file would otherwise be
# left in place and main_init.o (which includes it) would keep the old
# date.
build.h:
	@echo '/* Generated by GNUmakefile */' > $@
	@printf '#define\tBUILD_DATE "Created %s\\n"\n' "`date +%m/%d/%Y\ %H:%M:%S`" >> $@

#
# ---- the kernel ----------------------------------------------------
#
# ccc builds the whole kernel flat - no overlays - and mxld links it at
# the same fixed text base the native link used.  The entry header
# (uhdr.s) is first so its jump lands at 0x1000.  mem.s and the rest of
# the asm go *after* the C objects, though: mem.s remaps segment 1
# (0x1000-0x1fff) through MAP0[2] to reach user memory, and if it sat in
# that segment it would remap its own code out from under itself.
# Whitesmith's overlay link put mem.s at 0x46xx for the same reason.
#
# THE FINAL LINK IS AN ORDINARY LINK - text, then data, then bss, and
# -Ttext=0x1000 is the only address it is given.  It has no case for the
# u page and none for the init-only code; that the u page lands at 0xf000
# and the init-only code at _ebss is the work of the -r links above it,
# each of which has already decided what its sections are:
#
#	low.o		a plain fold.  text, data and bss come through as
#			they are, so the resident kernel lands where it
#			always has and _ebss is the end of textpad.o's bss.
#
#	upage.o		.data, all of it, at 0xf000 by -Tdata.  Text has
#			no address here to be placed at - 0xf000 is a data
#			address - so every .c that goes in has to be folded
#			to .data first, which is what rtc.o's and consts.o's
#			own rules are for.  Left as .text, mxld appends it to
#			the resident text and pads the text section out to
#			reach 0xf000, and the image stops fitting in the boot
#			loader's block list.
#
#	highmem.o	.data only: no text, no bss, and no address of
#			its own.  That shape is what -Shigh below parks
#			after bss, where expand_bufs() mints buffer headers
#			over it once init has returned.
#
# A new object for either place is the same bargain: fold it upstream,
# because this link has nowhere to put its text.
#
main_init.o: build.h

# upage.o is the u page: the leaf code (LEAF_AS) first at the base of
# segment USERSEG, then user.o, whose data is ustack (512 bytes, the
# system stack) below the u struct.  All of it is .data placed at 0xf000;
# the code must be .data, not .text - the loader reads text and then
# data, so text parked here would be overwritten.  u's register save area
# ends up at the base of the struct, which is where the firmware's trap
# entry leaves SP, so the kernel stack grows down into ustack.  textpad.o
# stays last in the main link so _ebss is the end of .bss and binit()'s
# buffer pool runs up to BUFWIN below u.
upage.o: $(LEAF_AS) $(LEAF_C) user.o
	$(MXLD) -r -Tdata=0xf000 -o upage.o $(LEAF_AS) $(LEAF_C) user.o

# highmem.o is the init-only code - main_init, cus, the MIO/console init
# in inits.s, and bcd2unix (unbcd.s) - folded into one data-only
# object with -Sdata.  The fold leaves the object with no text and no
# bss, and no address of its own - and -Shigh on the main link parks an
# unplaced object of exactly that shape after bss, so it sits at _ebss
# and binit()'s expand_bufs() mints buffer headers over it once init has
# returned.  The test is the object's shape, not a bit in its header:
# nothing marks highmem.o as init-only beyond having been folded.  No address is given
# here - the linker derives it from the resident .data/.bss, so it
# tracks the kernel as it grows without re-tuning.  bcd2unix's
# lifetime-0 .ds workspace rides here as data rather than pushing _ebss
# up from the resident .bss.
highmem.o: main_init.o cus.o inits.o unbcd.o
	$(MXLD) -r -Sdata -o highmem.o main_init.o cus.o inits.o unbcd.o

# win.o is the window-page cluster: the part of the resident kernel that
# can live at BUFSEG, in the very page bhold() maps buffers into, so its
# bytes stop competing with buffer headers for the space below.  The
# image is what costs here - sizeof(struct buf) is 21, so every 21 bytes
# below BUFWIN is one buffer - and this was 3915 of them.
#
# The criterion is execution, not linkage: code in this page runs only
# when no hold is up, so no object here may contain a hold site, and none
# may be reachable from the interrupt or boot paths (an interrupt can
# take the window, and getsb() takes one before a scheduler exists).
# Nothing here calls bhold(), and nothing in the closure of intrpt.s,
# uhdr.s and highmem.o calls into it - checked as a text-relocation graph
# rooted at the hold sites.  The residual gap is table dispatch, which a
# relocation graph cannot see: syssw is unreachable while holding because
# a syscall needs a trap and kernel mode takes none, which is exactly
# what sleep()'s bholdn panic protects.  Data here would be harmless, the
# page only shows buffer bytes during a hold, but every object chosen has
# none anyway, so -Sdata leaves the fold all-code.
#
# -Sdata is upage.o's reason, not an aesthetic: the loader writes text
# and then data in address order, so text parked up here is overwritten.
win.o: $(WIN_C)
	$(MXLD) -r -Sdata -Tdata=0xe000 -o win.o $(WIN_C)

# low.o is the resident kernel folded by a link of its own: the entry
# header, the C objects and the hand-written assembly, in the order the
# image needs them.  It is not a section fold like highmem.o's - a plain
# -r leaves text, data and bss as they are - so the final link places
# the one object where it packed the thirty-two, and _ebss lands at
# the end of textpad.o's .bss exactly as it did.
#
# The native build is why the fold exists: the words of that link are
# more than the shell there will carry, and the Makefile beside this one
# has the count and why make needs a shell at all.  The host build makes
# the same fold rather than a shorter list of its own, so the two link
# the same way and a difference between the images is a difference in
# the code.
low.o: uhdr.o $(KERNEL_C) intrpt.o mio.o textpad.o
	$(MXLD) -r -o low.o uhdr.o $(KERNEL_C) intrpt.o mio.o textpad.o

#
# the final link: four objects, in the order the image wants them, and
# nothing here is a special case (see the kernel section above).
# highmem.o precedes win.o because it is unplaced and parked at _ebss,
# and win.o precedes upage.o because 0xe000 is below 0xf000.
#
unix: $(OBJS)
	$(MXLD) -Ttext=0x1000 -Shigh -L$(HOSTROOT)/ccc/lib -lccc -lc -o unix low.o highmem.o win.o upage.o

#
# Generate the assembly ccc would assemble for a C file, for reading:
# "make system.s" writes system.s beside system.c, peepholed exactly as
# ccc builds system.o (-s is "compile only, keep the .s").  The output
# is the asz dialect ccc drives, not the A-Natural .s files KERNEL_AS
# names - those were hand-ported and checked in.
#
# The rule is idle while a normal build runs: .o is made straight from
# .c.  Once a .s has been generated, the %.o : %.s rule in
# GNUmakefile.inc assembles it in place of the .c - the same "the
# assembly wins" ordering the tree relies on everywhere else - so
# delete the .s to go back to compiling the .c.
#
%.s : %.c
	$(CCC) $(CCFLAGS) $(INC) -s $<

.PHONY: objects unix build.h
