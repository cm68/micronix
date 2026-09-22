#
# cross build for astpp - prints an AST file in readable form.  A debugging aid.
#
# cmd/astpp/GNUmakefile
#
# Built twice from the one set of sources.  The framework below builds
# it WITH ccc, FOR the Z80, which is the tool that runs on micronix and
# the one the driver execs out of /libexec.  host.mk at the foot builds
# the same sources with gcc -m32, to run here.
#
# Not in cmd/GNUmakefile's DIRS: it is not a command.  It installs into
# /libexec and nobody types it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= astpp
CLASS	= libexec
BINDIR	= /libexec

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
#
HOSTDEFS = -Dlinux
HOSTWARNS = -Wall
HOSTCFLAGS = -m32 -std=c99 -g -O0 $(HOSTDEFS) $(HOSTWARNS) -I. -I$(DEPTH)/lib/libccc -I$(DEPTH)/libexec/pass0
HOSTPROG = mxastpp

include $(DEPTH)/libexec/host.mk
