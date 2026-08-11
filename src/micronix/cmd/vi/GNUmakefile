#
# cross build for vi
#
# cmd/vi/GNUmakefile
#
# The only command here that needs a library beyond the ones ccc links
# by itself: hexdump and logmsg come out of lib/libutil.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= vi
CLASS	= cmd

LIBS	= $(DEPTH)/lib/libutil/libutil.a

include $(DEPTH)/GNUmakefile.inc
