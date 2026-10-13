#
# cross build for clean
#
# cmd/clean/GNUmakefile
#
# 'clean' is the program and also make's own 'clean' target, so the
# binary links under a name that does not collide and installs as
# /bin/clean - the INSTNAME mechanism, used the same way cpp uses it
# to install under another name.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= clean
CLASS	= cmd

TARGET	= cleanprog
INSTNAME = clean

CSRCS	= clean.c

MANSRCS	= clean.1

include $(DEPTH)/GNUmakefile.inc
