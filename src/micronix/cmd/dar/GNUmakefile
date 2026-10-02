#
# cross build for dar
#
# cmd/dar/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into dar.
#
# dar.ctl and dar.dist are data, not sources.  dar.dis is generated
# from them by "make disas" and is checked in so the reconstruction can
# be read against it without needing the tool.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= dar
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

DISAS	= $(DEPTH)/../tools/disas

dar.dis: dar.ctl dar.dist
	$(DISAS) -f dar.ctl dar.dist > $@
