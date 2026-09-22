#
# cross build for nm - lists the symbols in an object or an archive.
#
# cmd/nm/GNUmakefile
#
# The Z80 nm, built here with ccc.  Its host counterpart is mxnm - the
# symbol lister the cross build needs - and that is built from these
# same sources by the top GNUmakefile, into $(BIN) beside mxar, rather
# than by host.mk as the passes and the other tools are.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= nm
CLASS	= cmd

#
# wsobj.c, shared with asz and ld, lives in libutil.
#
LIBS	= $(DEPTH)/lib/libutil/libutil.a

include $(DEPTH)/GNUmakefile.inc
