#
# cross build for upm
#
# cmd/upm/GNUmakefile
#
# upm's code must sit above the CP/M TPA - a .com is loaded at 0100 and
# runs there, so upm's own text and data have to be higher.  Two things
# follow from that, and both are why this makefile does not just use the
# generic link rule:
#
#   - the crt is upmhead.s (start, entry, bios, tint), not lib/libu's
#     crt0, and it has to be the FIRST object so start lands at the text
#     base;
#   - the link drives ld directly with -Ttext=0xd500.  ccc would add
#     crt0.o (whose start would clash with upmhead's) and -Ttext=0x100
#     (which would load upm on top of the TPA).
#
# 0xd500 is not the original's d645: the reconstruction's text (the
# resident BDOS) compiles ~0x220 bytes larger than the Whitesmith
# original's, so its base sits that much lower.  The text ends at fc00-
# ish, which leaves the stack below ffff the room the original gave it.
#
# upm.ctl and upm.dis are data, not sources, and are not shipped by the
# update rule - the same as init's.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= upm
CLASS	= cmd

# the crt first, so start is at the text base; the syscall stubs before
# upm.o so they win over libu's RST-based ones
OBJS	= upmhead.o upmsys.o upm.o

include $(DEPTH)/GNUmakefile.inc

LIBC	= $(DEPTH)/lib/libc/libc.a
LIBU	= $(DEPTH)/lib/libu/libu.a

# upm.o is special.  The generic rule compiles upm.c to a single text
# segment; upm needs the original's backwards layout instead - the BDOS
# resident at the top of the address space, the CCP at 0x0100 where a
# .com clobbers it.  splitupm rewrites the generated assembly so the
# CCP's functions and buffers land in the data segment; everything else
# stays text.
upm.o: upm.c splitupm
	$(CCC) $(CCFLAGS) $(INC) -s -o upm.s upm.c
	./splitupm < upm.s > upm.split.s
	$(CCC) $(CCFLAGS) -c upm.split.s -o upm.o

# the generic rule links with ccc; upm links with ld, directly.  text
# ends at fc00-ish so the stack below ffff has the room the original
# gave it; data at 0100 (the CP/M TPA base).  The reconstruction's text
# is larger than the Whitesmith original's, so its base is lower than
# d645 - the TPA it leaves is still ~53k, which is the point.
$(TARGET): $(OBJS) $(TREELIBS)
	$(BIN)/mxld -o $(TARGET) -Ttext=0xd500 -Tdata=0x0100 \
		$(OBJS) $(LIBC) $(LIBU) $(LIBC)
