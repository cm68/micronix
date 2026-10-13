#
# cross build for wc
#
# cmd/wc/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into wc.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= wc
CLASS	= cmd

#
# The distribution's three counters are this program under other
# names; main() tells them apart by looking at argv[0], as man does
# for help.
#
ALIAS	= chars lines words

CSRCS	= wc.c

MANSRCS	= chars.1 lines.1 wc.1 words.1

include $(DEPTH)/GNUmakefile.inc
