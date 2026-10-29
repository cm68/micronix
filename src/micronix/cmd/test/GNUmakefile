#
# cross build for test
#
# cmd/test/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into test.
#
# [ is this same binary under a second name.  There are no symbolic
# links here, so install copies it - see INSTALLALIAS in
# GNUmakefile.inc, which man and ed use for help and e.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= test
CLASS	= cmd

CSRCS	= test.c operators.c

HSRCS	= operators.h

MANSRCS	= test.1

ALIAS	= [

include $(DEPTH)/GNUmakefile.inc
