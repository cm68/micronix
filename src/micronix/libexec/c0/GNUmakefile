#
# cross build for c0 - the parser: C in, an AST and a data file out.
#
# cmd/c0/GNUmakefile
#
# Built twice from the one set of sources.  The framework below builds
# it WITH ccc, FOR the Z80, which is the pass that runs on micronix and
# the one the driver execs out of /libexec.  host.mk at the foot builds
# the same sources with gcc -m32, to run here.
#
# Not in cmd/GNUmakefile's DIRS: a pass is not a command.  It installs
# into /libexec and nobody types it.
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
HOSTPROG = mxc0

include $(DEPTH)/libexec/host.mk
