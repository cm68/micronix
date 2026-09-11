#
# cross build for the kernel
#
# sys/GNUmakefile
#
# "all" builds the kernel: unix, at a fixed text base of 0x1000.  ccc
# has no -tb, no overlay -r and no -x, so the link is done by calling
# mxld directly with -Ttext=0x1000.  The native Makefile beside this
# one stages low.o/io.o/high.o overlays and links them with
# "link -tb0x1000 -r"; the flat link here is the same image without the
# overlays, in one pass.
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

# build.h is written by the ccc survey below; GNUmakefile.inc's clobber
# takes GENERATED away.
GENERATED = build.h

# the tree's ccc and its linker - GNUmakefile.inc's ?= defaults point
# at an installed mxccc that is not here, so these are set before the
# include.
CCCTREE ?= $(HOME)/src/ccc
CCC      = $(CCCTREE)/desthost/bin/ccc
MXLD     = $(CCCTREE)/desthost/libexec/mxld

# the kernel's objects, in link order: the entry header (uhdr.s) first
# so its jump lands at 0x1000.  inout.s is here though the native
# Makefile's AS_OBJS never listed it: memdev.c, multio.c, mw.c, dj.c
# and cus.c all call in()/out().  makedate.c (a host program), mset.c
# (no hardware interface) and lock.c (dead) are left out.
KERNEL_AS = uhdr.o mem.o sub8.o inout.o intrpt.o mio.o inits.o textpad.o
KERNEL_C  = mount.o access.o balloc.o cio.o create.o exec.o fio.o fork.o \
	ialloc.o iget.o imap.o link.o malloc.o name.o open.o queue.o \
	rdwri.o reg.o sig.o sleep.o subc.o swap.o sys1.o sys2.o system.o time.o \
	trap.o tty.o uio.o con.o dj.o memdev.o multio.o mw.o main.o cus.o
OBJS     = $(KERNEL_AS) user.o $(KERNEL_C)
TREELIBS =

include $(DEPTH)/GNUmakefile.inc

#
# what "all" would produce if the overlay link existed
#
objects: $(OBJS)

# ---- the kernel through ccc ---------------------------------------
#
# A survey, not a build.  It compiles each kernel source with the Z80
# C compiler in ~/src/ccc and says which ones it got through; it does
# not link, and what comes out is not a kernel.  The point is to find
# out what that compiler cannot yet do, using the largest body of
# Whitesmiths-era C in the tree as the sample.
#
# Objects go in their own directory.  The native build leaves its .o
# files here, built by cc for the same machine, and two compilers'
# output under one name in one directory is a trap - see the same
# arrangement in ccc's own src/tools.
#
# The .s files are not in this: they are A-Natural for as(1), which is
# a different assembler and a different syntax from the asz that ccc
# drives.  makedate.c is not either - it is a host program that writes
# build.h, not part of the kernel.
#
# mset.c is out too, and for a better reason than it not compiling.
# It is in none of LOW_C, IO_C or HIGH_C in the Makefile beside this,
# so the native build does not compile it either; the only mention it
# gets there is a stale dependency line.  It reads the mio line
# control register through MBASE, LCONTROL, BAUDREG, MU_WORD8,
# MU_STOP2, MU_PARITY and MU_EVEN, and of those only MBASE is defined
# anywhere in the tree - in multio.c, which is a different translation
# unit.  So it cannot compile without inventing a hardware interface,
# and a survey that counts it is measuring the wrong thing.  What was
# wrong with it that CAN be told - an undeclared variable and a use of
# rates[] before its definition - is fixed and commented in the file.
#
# It reports and exits 0.  Three of the thirty-nine do not compile
# today, so making it a gate would only mean it always fails; drop the
# "; true" at the end when that stops being true.  It was twelve until
# the source was gone through; what is left is the compiler's:
#
#   cus   *p++ = ((unsigned) djcomm >> 0), where the ">> 0" folds away
#         and leaves a symbol's ADDRESS being stored into a byte.  No
#         rule builds it: (ASSIGN:byte (DEREF:byte) (SYMREFptr)).
#   mw    ichan[0] = &cmd - a symbol's address stored through a
#         pointer held in a register.  Same shape one width up:
#         (ASSIGN:short (DEREF:short) (SYMREFptr)).
#   main  char (*buffer)[512] - a pointer to an array.  c0 reads the
#         declarator as an array, so "buffer = 0" is "need lvalue" and
#         "buffer[1][2]" is "bad index".  Three lines reproduce it.
#
#
# SURVEYCC and not CCC.  GNUmakefile.inc is included above and sets
# CCC ?= /usr/local/bin/ccc, so a "CCC ?=" here is a no-op and the
# survey quietly measured whatever was last INSTALLED rather than what
# is in the tree.  That is the wrong compiler to be asking about: the
# point of this target is to find out what the one being worked on
# cannot do, and an install can be days behind.
#
# It cost real confusion once - three files reported as failing here
# that compiled by hand with the tree's compiler, because the two were
# not the same program.
#
CCCTREE  ?= $(HOME)/src/ccc
SURVEYCC ?= $(CCCTREE)/desthost/bin/ccc

#
# The linker and the object tools are not on the path and are not in
# desthost/bin either - bin holds the driver and the things a user
# runs, and the driver reaches into libexec for the passes and mxld.
#
MXLD     ?= $(CCCTREE)/desthost/libexec/mxld
MXSIZE   ?= $(CCCTREE)/desthost/bin/mxsize
MXNM     ?= $(CCCTREE)/desthost/bin/mxnm

CCCDIR    = cccobj
CCCOUT    = kernel.o
CCCFLAGS  = -O -c -i../include -I.

CCCSRCS   = $(filter-out makedate.c mset.c,$(wildcard *.c))

#
# main.c includes it, the native build generates it with makedate, and
# makedate is itself a Micronix program - it wants types.h, std.h and
# a struct time this host has not got.  All the header carries is the
# date, so the survey writes its own rather than fail main.c for a
# reason that has nothing to do with the compiler.
#
build.h:
	@echo '/* Generated by GNUmakefile for the ccc survey */' > $@
	@printf '#define\tBUILD_DATE "Created %s\\n"\n' "`date +%m/%d/%Y`" >> $@

ccc-try: build.h
	@mkdir -p $(CCCDIR)
	@ok=0; bad=0; fail=""; \
	for f in $(CCCSRCS); do \
		b=`basename $$f .c`; \
		printf "%-12s" "$$b: "; \
		if $(SURVEYCC) $(CCCFLAGS) -o $(CCCDIR)/$$b.o $$f \
				> $(CCCDIR)/$$b.log 2>&1; then \
			echo "ok"; ok=`expr $$ok + 1`; \
			rm -f $(CCCDIR)/$$b.log; \
		else \
			echo "FAIL"; bad=`expr $$bad + 1`; fail="$$fail $$b"; \
		fi \
	done; \
	echo "-------------------------------------------------------"; \
	echo "$$ok compiled, $$bad did not:$$fail"; \
	echo "(the ones that failed left $(CCCDIR)/<name>.log)"; \
	true

#
# ---- linking what compiled -----------------------------------------
#
# A partial link - "-r", relocatable out - because the survey does not
# compile the whole kernel yet, and a final link would fail on the
# symbols the missing files define, which says nothing about whether
# the objects agree with each other.  What is left undefined is those
# files' routines and the compiler's own frame helpers, which live in
# libccc.a and are not offered here.
#
# The bss numbers are the interesting part.  A header of this vintage
# writes "struct user u;" and every file including it gets its own u,
# and wsld folds those together and takes the duplicated bytes back.
# But where one file also initializes the object, its copy is in data
# and the rest are in bss, and those are NOT one object to be merged -
# the initialized one is the definition and the others are a mistake
# the header should have written extern.  wsld refuses that pair, and
# refusing it is what found the eight in this kernel.
#
# With those declared extern the duplicates never reach the linker at
# all: the compiler stops emitting them.  So "reclaimed 0" is the
# right answer here and not a sign the merge is broken - there is
# nothing left for it to merge but the .ebss sentinel.
#
# wssize prints text, data, bss, total, total-in-hex, name; bss is the
# third, on the second line under the header.
#
ccc-link:
	@test -d $(CCCDIR) || { echo "no $(CCCDIR) - run 'make ccc-try' first"; exit 1; }
	@rm -f $(CCCDIR)/$(CCCOUT)
	@objs=`ls $(CCCDIR)/*.o 2>/dev/null | grep -v '$(CCCOUT)$$'`; \
	test -n "$$objs" || { echo "no objects in $(CCCDIR)"; exit 1; }; \
	n=`echo $$objs | wc -w`; \
	in=0; \
	for o in $$objs; do \
		b=`$(MXSIZE) $$o | awk 'NR==2{print $$3}'`; \
		in=`expr $$in + $$b`; \
	done; \
	echo "linking $$n objects, $$in bytes of bss between them"; \
	echo "-------------------------------------------------------"; \
	if $(MXLD) -r -o $(CCCDIR)/$(CCCOUT) $$objs; then \
		out=`$(MXSIZE) $(CCCDIR)/$(CCCOUT) | awk 'NR==2{print $$3}'`; \
		echo "-------------------------------------------------------"; \
		echo "linked bss $$out, was $$in, reclaimed `expr $$in - $$out`"; \
		echo "u is at `$(MXNM) $(CCCDIR)/$(CCCOUT) | awk '$$5==\"_u\"{print $$1, $$2}'`"; \
	else \
		echo "-------------------------------------------------------"; \
		echo "mxld failed"; exit 1; \
	fi

#
# the survey's objects live in their own directory, so removing them is
# a directory removal and not something clean's "rm -f" can express.
#
ccc-clean:
	rm -rf $(CCCDIR)

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
main.o: build.h

unix: $(OBJS)
	$(MXLD) -Ttext=0x1000 -L$(CCCTREE)/desthost/lib -lccc -lc -o unix uhdr.o user.o $(KERNEL_C) mem.o sub8.o inout.o intrpt.o mio.o inits.o textpad.o

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

.PHONY: objects ccc-try ccc-link ccc-clean unix
