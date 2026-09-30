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

# The prose.  The source tar carries $(SRCS), which is the *.c/*.h/*.s/*.1
# wildcards plus $(EXTRA), and none of these is any of those - so without
# this the kernel ships to /usr/src/sys with its code and none of the
# writing that explains it.  BUILDING is how this directory is built and
# TODO is what is left to do in it, so both belong here; the project's
# design notes used to as well, and they are the repository's docs/ now,
# which the top-level update ships to /usr/src/docs (GNUmakefile above).
#
EXTRA	= BUILDING TODO

# build.h is written by the rule below, and unix.link is the half-built
# image unix is made from; GNUmakefile.inc's clobber takes GENERATED
# away, so neither is left lying in the directory for the next build to
# include or to copy.  A driver is built twice - linked into unix.link
# and linked again as a module - so its objects, .mod and .page are not
# in OBJS; they ride GENERATED for the same reason.  unix itself is
# $(TARGET), which clobber removes.
MODS	= dj mw ide ncr
GENERATED = build.h unix.link $(MODS:%=%.mod) $(MODS:%=%.page) \
	$(MODS:%=%.o) $(MODS:%=%hdr.o)

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
#
# They have to be here and not in a library, because a driver module is
# linked against the kernel and nothing else: whatever the compiler emits
# a call to has to be resident, or the module does not link.  qinc is the
# one that showed it - a UINT32 postfix ++ in a driver's wait loop is a
# call.  qmul is here for the same reason, and it cannot be avoided by
# writing the multiply out longhand: which of the runtime's multiplies an
# expression costs is the compiler's to choose, and a driver's geometry
# math multiplies two 16-bit counts into a 32-bit block number.
# (A cheaper qmul for that case was tried and does not exist: qmul is 49
# bytes and almost all of it is the shift-and-add body, so a 16-turn
# version is the same code with a smaller counter, and the C version
# costs 119 - every access to a 32-bit local is a call.)
LEAF_LIBC = csv.o idiv.o imul.o qadd.o qsub.o \
	qand.o qor.o qcmp.o qshl.o qsar.o qshr.o \
	qmul.o qinc.o \
	qldst.o tramp.o swdisp.o swdispw.o oarg.o
LEAF_AS   = mem.o inout.o sub8.o unixtobcd.o $(LEAF_LIBC)
# Kernel C that also lives in the u page: the read-only tables (syssw,
# biosw, ciosw) and the bus lock two driver modules share.  rtc.o was
# here too - it is pure code with no static data, so it could be folded
# to .data and parked like the runtime - until the leaf code grew past
# what the page holds.  Its callers are mount() and sys2.c and both are
# resident, so nothing a driver module links against reaches it and it
# is ordinary resident C again (KERNEL_C).
LEAF_C    = consts.o bus.o

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

# bus.c is the DMA bus lock mw.c and dj.c both call, which is what puts
# it in the u page: the two are modules and cannot call into each other,
# and the u page is resident, so a module finds busget/busgive in the
# kernel's symbol table (-Aunix) the way it finds qadd and the rest of
# the leaf runtime.  It is folded like the two above - it is code, and
# without the fold the loader's data pass would write it over the u page.
bus.o: bus.c
	$(CCC) $(CCFLAGS) $(INC) -c -o bus.rel $<
	$(MXLD) -r -Sdata -o bus.o bus.rel
	rm -f bus.rel

KERNEL_AS = uhdr.o intrpt.o mio.o textpad.o highmem.o
KERNEL_C  = mount.o balloc.o cio.o create.o exec.o fio.o fork.o \
	ialloc.o iget.o imap.o malloc.o name.o queue.o \
	reg.o sig.o sleep.o subc.o swap.o sys1.o system.o time.o \
	trap.o tty.o uio.o multio.o dlabel.o ovl.o main.o rtc.o
WIN_C     = open.o sys2.o rdwri.o memdev.o link.o access.o
HIGHMEM   = main_init.o cus.o inits.o djinit.o ncrinit.o
# dj, mw, ide and ncr are not here: they are driver modules, linked at
# OVLBASE and stamped into the slot (the module rules below).  Their
# header objects are named on the module links and no longer in the
# kernel either - a module's header is the module's own first bytes.
# What IS here from a module is its init (djinit, ncrinit): it runs once,
# from the kernel's side of the seam, and folding it into highmem.o is
# what keeps it out of the driver's one 4K page.
OBJS     = $(KERNEL_AS) $(LEAF_AS) $(LEAF_C) user.o upage.o $(KERNEL_C) $(WIN_C) $(HIGHMEM) low.o win.o $(OVLSLOT)
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
# The overlay frame (sys/ovl.h): OVLSEG is the one segment of the
# kernel's address space a driver module is mapped into, and OVLBASE is
# where that is.  One variable, passed to the C and used by the module
# links below, because the two have to agree and a segment that moved on
# one side only would be a module mapped nowhere.  The segment is a
# property of the build and not a choice: it is the page just past the
# end of the resident text, so it moves when the text grows past its
# page, and this is the place it moves.
#
# The frame is opened by OVLDATA, one page above OVLBASE: the final link
# is told the data segment starts there and ovlslot.o is linked at
# OVLBASE, so the page between them is the slot - text with nothing in it
# but the slot object, data below it at its own page, and so a page that
# can be overwritten with a driver without disturbing anything.  ovlslot.o
# is what makes the text reach that far: the linker sizes the text segment
# by its last object, so a slot that were only a gap in the addresses
# would be a slot the file has no bytes for and setdev nothing to write
# onto.
#
OVLSEG  = 9
OVLBASE = $(shell printf 0x%x $$(( $(OVLSEG) * 4096 )))
OVLDATA = $(shell printf 0x%x $$(( ($(OVLSEG) + 1) * 4096 )))
PAGESZ  = 4096
OVLSLOT = ovlslot.o
CCFLAGS += -DOVLSEG=$(OVLSEG)

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
#			to .data first, which is what consts.o's own rule is
#			for.  Left as .text, mxld appends it to
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
# in inits.s, bcd2unix (unbcd.s), and the disk drivers' own bring-up
# (djinit.c and its siblings as they are split out) - folded into one
# data-only object with -Sdata.  The fold leaves the object with no text and no
# bss, and no address of its own - and -Shigh on the main link parks an
# unplaced object of exactly that shape after bss, so it sits at _ebss
# and binit()'s expand_bufs() mints buffer headers over it once init has
# returned.  The test is the object's shape, not a bit in its header:
# nothing marks highmem.o as init-only beyond having been folded.  No address is given
# here - the linker derives it from the resident .data/.bss, so it
# tracks the kernel as it grows without re-tuning.  bcd2unix's
# lifetime-0 .ds workspace rides here as data rather than pushing _ebss
# up from the resident .bss.
#
# A driver's init belongs here and not in the driver because the driver
# is what has to fit a page.  A driver is linked as a module at OVLBASE
# and copied into one 4K segment; its init runs once, before anything can
# reach it, so the page it occupies is the page it holds for the whole
# boot.  Split out, the init is reclaimed instead, and the page holds
# only what running the driver needs.
highmem.o: main_init.o cus.o inits.o unbcd.o djinit.o ncrinit.o
	$(MXLD) -r -Sdata -o highmem.o main_init.o cus.o inits.o unbcd.o djinit.o ncrinit.o

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

# ovlslot.o is the overlay slot (sys/ovlslot.s): one page of text at
# OVLBASE, which setdev later writes the root driver's page onto.  The
# -r link is what holds it at that address.  Assembled, the object is an
# ordinary one - the slot is wherever the assembler's section starts,
# which is the assembler's business, not the kernel's - and -Ttext says
# where that is, so the object's own header carries the page and mxld's
# final link places it there instead of packing it after the resident
# text.  win.o and upage.o are the same trick on the data side; there is
# no way to say it in the assembly, because asz's .align counts from the
# start of the section it is assembling and this address is a property
# of the link.
ovlslot.o: ovlslot.s
	$(CCC) $(CCFLAGS) -c -o ovlslot.rel $<
	$(MXLD) -r -Ttext=$(OVLBASE) -o ovlslot.o ovlslot.rel
	rm -f ovlslot.rel

#
# the final link: five objects, in the order the image wants them, and
# nothing here is a special case (see the kernel section above).
# highmem.o precedes win.o because it is unplaced and parked at _ebss,
# win.o precedes upage.o because 0xe000 is below 0xf000, and ovlslot.o
# is last because it is the top of the text.
#
# -Tdata is what opens the overlay frame: the data segment starts at
# OVLDATA, one page above the slot, and the linker pads the text out to
# reach it.  Without it the data would start at the end of the resident
# text, which is inside the slot.
#
# The output is unix.link and not unix, because unix is the *bootable*
# image: this link has an empty slot and no modules behind it, so it
# cannot be booted.  It is kept under its own name because it is what the
# module links are resolved against (-A below), and because unix - the
# thing create_vol copies to /micronix and the tracer resolves symbols
# against - is this file with the module pages on the end and a driver
# stamped into the slot.
unix.link: $(OBJS)
	$(MXLD) -Ttext=0x1000 -Tdata=$(OVLDATA) -Shigh -L$(HOSTROOT)/ccc/lib -lccc -lc -o unix.link low.o highmem.o win.o upage.o $(OVLSLOT)

#
# Driver modules, and the bootable image they go into.
#
# A driver is built twice while the kernel still holds it: linked in, as
# it always was, and linked a second time as a module.  The objects are
# the same ones; only the link differs.  What makes the two one driver is
# that a module's first bytes are its header (sys/ovl.h), which is what
# the kernel reads at OVLBASE and which tells it everything it will ever
# know about the driver.
#
# The header is an object of its own, named first, and the link is
# -Sdata.  That flag folds every section into the data segment, and an
# object's data is placed in the order the objects are named on the link
# line - so the header's 28 bytes are the page's first 28 and nothing the
# driver holds can get in front of them.  The same ordering is why the
# header object holds the struct and nothing else, and why the name in it
# is a character list: sys/ovl.h has what a string would cost.
#
# -Aunix is the other direction.  A module is linked against the kernel's
# own symbol table, so it can name anything in the kernel and be given a
# real address; nothing in the kernel names the module, which is what
# lets the kernel be linked with no disk drivers at all.  The image it is
# linked against has to be one without modules in it, which is why the
# pages go on a copy (the rule below) rather than onto unix.link - a
# second make would otherwise stack a second set of pages on the first.
#
# -s drops the symbol table.  A module is linked against and never linked
# from, so it has no use for one and the page should not carry it.
#
# While the drivers are still in $(KERNEL_C) the linker says so - "-A:
# _djopen: the module defines this too", once per symbol - and that
# warning is the measure of what is left to do: it goes away when the
# driver leaves the kernel.
#
$(MODS:%=%.mod): %.mod: %hdr.o %.o unix.link
	$(MXLD) -s -Sdata -Tdata=$(OVLBASE) -Aunix.link -o $@ $*hdr.o $*.o

#
# One module is one page.  What the link produced is a 16 byte object
# header followed by the page's bytes - all of them, because -Sdata puts
# every section there and -s leaves nothing after - so the page is what
# follows the header.  dd takes it and truncate brings a short module up
# to the 4K the kernel will read, which is the ordinary case.  A module
# that does not fit is too big for its page and is worth stopping on
# rather than silently halving.
#
$(MODS:%=%.page): %.page: %.mod
	@if [ `wc -c < $<` -gt $$((16 + $(PAGESZ))) ]; then \
		echo "$< is larger than a page"; exit 1; fi
	dd if=$< of=$@ bs=1 skip=16 count=$(PAGESZ) 2>/dev/null
	truncate -s $(PAGESZ) $@

#
# The bootable kernel: the link, the module pages stacked above its
# extent, and the root driver stamped into the slot.
#
# The pages go at the first page boundary past the object, which is what
# setdev() calls modbase and computes for itself out of the object's own
# header - text, data and symbol table all three.  The two agree because
# this pads the file and the file is exactly that extent, and a
# disagreement is not silent: setdev reads the major out of each page and
# dies if it finds no module for the root device.
#
# The stamp is the last step and it is what makes the image bootable: the
# kernel is linked with a page-sized hole where its root driver goes, and
# setdev fills it with the module whose header serves the major that
# _rootdev already names.  -i is that mode - it stamps the driver for the
# kernel's own root device and writes no device numbers, so the default
# in main.c is the only place it is stated.
#
unix: unix.link $(MODS:%=%.page)
	cp unix.link $@
	@sz=`wc -c < $@`; \
	 truncate -s $$(( (sz + $(PAGESZ) - 1) / $(PAGESZ) * $(PAGESZ) )) $@
	cat $(MODS:%=%.page) >> $@
	$(SETDEV) -i $@

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
