#
# cross build for the kernel
#
# sys/GNUmakefile
#
# "all" does not build a kernel yet, and does not pretend to.  The
# kernel is not a program ccc can drive to completion: Makefile beside
# this one links three overlays at a fixed text base
#
#	link -tb0x1000 -r -o unix low.o io.o -lm.a high.o
#
# and compiles two of the files with overlay numbers (-x0, -x7).  ccc
# has no -tb, no -r and no -x, so the link has to be done by calling
# wsld directly, with the segment layout worked out.  That is its own
# piece of work; until it is done this builds the objects, which is
# the part that can be checked, and says so rather than reporting a
# kernel it did not produce.
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

# no ccc driver can link this yet - see above
TARGET	=

# the kernel includes its own headers, not the C library's.  -i, as
# the native Makefile beside this has always done.
INC	= -i$(DEPTH)/include

# build.h is written by the ccc survey below; GNUmakefile.inc's clobber
# takes GENERATED away.
GENERATED = build.h

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
# runs, and the driver reaches into lib for the passes and for wsld.
#
WSLD     ?= $(CCCTREE)/desthost/lib/wsld
WSSIZE   ?= $(CCCTREE)/desthost/bin/wssize
WSNM     ?= $(CCCTREE)/desthost/bin/wsnm

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
		b=`$(WSSIZE) $$o | awk 'NR==2{print $$3}'`; \
		in=`expr $$in + $$b`; \
	done; \
	echo "linking $$n objects, $$in bytes of bss between them"; \
	echo "-------------------------------------------------------"; \
	if $(WSLD) -r -o $(CCCDIR)/$(CCCOUT) $$objs; then \
		out=`$(WSSIZE) $(CCCDIR)/$(CCCOUT) | awk 'NR==2{print $$3}'`; \
		echo "-------------------------------------------------------"; \
		echo "linked bss $$out, was $$in, reclaimed `expr $$in - $$out`"; \
		echo "u is at `$(WSNM) $(CCCDIR)/$(CCCOUT) | awk '$$5==\"_u\"{print $$1, $$2}'`"; \
	else \
		echo "-------------------------------------------------------"; \
		echo "wsld failed"; exit 1; \
	fi

#
# the survey's objects live in their own directory, so removing them is
# a directory removal and not something clean's "rm -f" can express.
#
ccc-clean:
	rm -rf $(CCCDIR)

.PHONY: objects ccc-try ccc-link ccc-clean
