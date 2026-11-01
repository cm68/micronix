#
# cross build for cc
#
# cmd/cc/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into cc.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= cc
CLASS	= cmd

CSRCS	= cc.c

MANSRCS	= cc.1

#
# The Whitesmiths preprocessor has no source; it is shipped as a
# binary and installed under the name this driver invokes, wpp, so
# that cpp can be the tree's own preprocessor.  See install-wpp.
#
EXTRA	= wpp.dist

include $(DEPTH)/GNUmakefile.inc

#
# The vendor preprocessor goes in beside the cc that runs it.  The
# generic install copies TARGET and the pages; this is the one extra
# file, and as a prerequisite of install it lands on a plain
# "make install".
#
install: install-wpp
install-wpp:
	mkdir -p $(INSTDIR)
	rm -f $(INSTDIR)/wpp
	cp wpp.dist $(INSTDIR)/wpp
	chmod 755 $(INSTDIR)/wpp
