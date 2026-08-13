#
# cross build for init
#
# cmd/init/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into init.
#
# init.ctl and init.dist are data, not sources, and are not shipped by
# the update rule - which is what was true before this was converted.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= init
CLASS	= cmd

# process 1 lives in /etc, not /bin
BINDIR	= /etc

include $(DEPTH)/GNUmakefile.inc
