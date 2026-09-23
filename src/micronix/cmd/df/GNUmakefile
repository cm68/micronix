#
# cross build for df
#
# cmd/df/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into df.
#
# df.ctl and df.dist are data, not sources, and are not shipped by the
# update rule - the same as init's and man's.  df.dis is generated from
# them by "make df.dis" and is checked in so that the reconstruction can
# be read against it without needing the tool.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= df
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

#
# The disassembly of the original, regenerated from the control file.
# Not part of "all" - it needs src/tools/disas, which is a host program,
# and the output is checked in.
#
DISAS	= $(DEPTH)/../tools/disas

df.dis: df.ctl df.dist
	$(DISAS) -f df.ctl df.dist > $@
