#
# cross build for upm
#
# cmd/upm/GNUmakefile
#
# upm's code must sit above the CP/M TPA - a .com is loaded at 0100 and
# runs there, so upm's own text and data have to be higher.  Three things
# follow from that, and all three are why this makefile does not just use
# the generic rules:
#
#   - the crt is upmhead.s (start, entry, bios, tint), not lib/libu's
#     crt0, and it has to be the FIRST object so start lands at the text
#     base;
#   - the link drives ld directly with -Ttext=0xd500.  ccc would add
#     crt0.o (whose start would clash with upmhead's) and -Ttext=0x100
#     (which would load upm on top of the TPA);
#   - the source is two translation units.  The original divides upm at
#     the TPA boundary - the CCP is data at 0100, where a loaded .com
#     clobbers it and a warm boot puts it back; the BDOS resident is
#     text at the top.  ccc emits one object per .c with every function
#     in .text, so the halves have to be separate sources, and each is
#     folded to a single segment with -r -Stext / -r -Sdata before the
#     link.  upm.h says which half owns what.
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
# the upm halves so they win over libu's RST-based ones
OBJS	= upmhead.o upmsys.o upm_res.o upm_ccp.o

include $(DEPTH)/GNUmakefile.inc

LIBC	= $(DEPTH)/lib/libc/libc.a
LIBU	= $(DEPTH)/lib/libu/libu.a

# The two halves.  -r -Stext folds the resident object's every section
# into its text segment and -r -Sdata does the same for the CCP's data
# segment, which is what lets the -T options below put them at 0xd500
# and 0100 with the linker alone.  Rebuild both when upm.h changes,
# since it is what tells each half which symbols the other defines.
upm_res.o: upm_res.c upm.h
	$(CCC) $(CCFLAGS) $(INC) -c upm_res.c -o upm_res.rel
	$(BIN)/mxld -r -Stext -o upm_res.o upm_res.rel

upm_ccp.o: upm_ccp.c upm.h
	$(CCC) $(CCFLAGS) $(INC) -c upm_ccp.c -o upm_ccp.rel
	$(BIN)/mxld -r -Sdata -o upm_ccp.o upm_ccp.rel

# the generic rule links with ccc; upm links with ld, directly.  text
# ends at fc00-ish so the stack below ffff has the room the original
# gave it; data at 0100 (the CP/M TPA base).  The reconstruction's text
# is larger than the Whitesmith original's, so its base is lower than
# d645 - the TPA it leaves is still ~53k, which is the point.
#
# upm_res.o contributes no data and upm_ccp.o no text, so the CCP's data
# segment is the first one in the link and lands at 0100 exactly.
$(TARGET): $(OBJS) $(TREELIBS)
	$(BIN)/mxld -o $(TARGET) -Ttext=0xd500 -Tdata=0x0100 \
		$(OBJS) $(LIBC) $(LIBU) $(LIBC)
