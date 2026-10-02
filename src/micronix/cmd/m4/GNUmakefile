#
# cross build for m4
#
# cmd/m4/GNUmakefile
#
# m4eval.c is the hand-written eval parser (its header explains why);
# m4y.y is the original yacc grammar, shipped for reference.  -Dunix is
# what turns on <signal.h>'s SIGHUP/SIGINT and makes the "unix" macro
# predefined - the M4 variant (uppercase names, [ ] quotes) is left off.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= m4
CLASS	= cmd

OBJS	= m4.o m4eval.o

include $(DEPTH)/GNUmakefile.inc

CCFLAGS += -Dunix
SRCS += m4y.y
