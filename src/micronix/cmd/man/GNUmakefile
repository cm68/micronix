#
# cross build for man
#
# cmd/man/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in this
# directory, linked into man.
#
# man.ctl and man.dist are data, not sources, and are not shipped by the
# update rule - the same as init's.  man.dis is generated from them by
# "make disas" and is checked in so that the reconstruction can be read
# against it without needing the tool.
#
# help is man under another name: main() looks for "help" inside argv[0].
# The link is made at install time rather than built, so there is one
# binary and not two.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= man
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

#
# The disassembly of the original, regenerated from the control file.
# Not part of "all" - it needs src/tools/disas, which is a host program,
# and the output is checked in.
#
DISAS	= $(DEPTH)/../tools/disas

man.dis: man.ctl man.dist
	$(DISAS) -f man.ctl man.dist > $@
