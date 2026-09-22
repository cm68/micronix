#
# cross build for form
#
# cmd/form/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into form.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= form
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

#
# The host build: mxform, the cross formatter.  mxman execs it by name,
# so it is installed beside mxman.
#
HOSTSRCS = form.c
HOSTCFLAGS = -m32 -std=gnu89 -g -O0 -Dlinux -Wall -I.
HOSTPROG = mxform
HOSTBIN  = bin
include $(DEPTH)/libexec/host.mk
