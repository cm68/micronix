#
# cross build for pass0
#
# libexec/pass0/GNUmakefile
#
# ccc's first pass: the lexer and the normalizer, which emits the .x
# lexeme stream that c0 reads.  Not a preprocessor, whatever name it
# used to carry.  The sources inside are still cpp.c and cpp.h,
# because that is what they have always been called.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= pass0
CLASS	= libexec

#
# Not /bin.  ccc runs this; nobody types it.
#
BINDIR	= /libexec

#
# The driver runs it by the name pass0, and every other pass is called
# after itself - ccc.c builds this path as libexecdir + "/pass0".  The
# cross build used to leave a /libexec/cpp that nothing ever executed.
#
INSTNAME = pass0

#
# The compiler's own runtime: fmtlong and libutil.  Every pass calls
# them and none of them contains them, which is what left this
# undefined at _fmtlong and _fmtstr before lib/libccc existed.
#
LIBS	= $(DEPTH)/lib/libccc/libccc.a

#
# Spelled out, because the wildcard is wrong here in three ways: it
# would sweep in dbgtags.c, which cpp.c includes rather than compiles;
# and mkkw.c and xdump.c, which are programs of their own with their
# own main - the Z80 link said so, twice over, as duplicate _main.
#
CSRCS = cpp.c lex.c io.c macro.c kw.c emit.c util.c kwtab.c \
	lexdata.c tdsrc.c cfold.c knr.c norm.c filtutil.c

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
#
#
# c89 and not gnu89, and the builtin-mismatch warning off:
# cpp declares strdup itself, as it must - it has to compile with a
# compiler whose library does not.
#
HOSTWARNS = -Wall -Wno-builtin-declaration-mismatch -Werror=declaration-after-statement
HOSTCFLAGS = -m32 -std=c89 $(HOSTDEBUG) $(HOSTDEFS) $(HOSTWARNS) -I. -I$(DEPTH)/lib/libccc
HOSTSRCS = $(CSRCS)
HOSTPROG = mxpass0

include $(DEPTH)/libexec/host.mk
