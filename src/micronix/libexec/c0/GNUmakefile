#
# cross build for c0 - the parser: C in, an AST and a data file out.
#
# cmd/c0/GNUmakefile
#
# PROTOTYPE.  The compiler is moving into this tree, and this is the
# shape it will have when it arrives: one directory per pass and per
# tool.  The sources live here.  They were symlinks into ~/src/ccc until
# the fork; this tree holds its own copies and is where they are
# edited now.
#
# Not in cmd/GNUmakefile's DIRS.  These are host programs today, built
# by gcc -m32 in the ccc tree; building them here with ccc means
# building the compiler for the Z80, which is the point of moving them
# but is not something that works yet.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= c0
CLASS	= libexec

#
# Not /bin.  ccc runs this; nobody types it.
#
BINDIR	= /libexec

#
# The compiler's own runtime: fmtlong and libutil.  Every pass calls
# them and none of them contains them, which is what left this
# undefined at _fmtlong and _fmtstr before lib/libccc existed.
#
LIBS	= $(DEPTH)/lib/libccc/libccc.a

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
#
#
# Spelled out: dbgtags.c is included by pass1.c, not compiled.
#
HOSTSRCS = pass1.c error.c lexread.c fold.c pfx.c post.c name.c \
	   init.c istream.c eutil.c expr.c parse.c pblock.c pstmt2.c \
	   swcnt.c decl.c type.c tparse.c declare.c outast.c outh.c \
	   outfn.c regalloc.c util.c
HOSTPROG = c0

include $(DEPTH)/libexec/host.mk
