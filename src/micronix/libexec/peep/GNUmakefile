#
# cross build for peep - the peephole optimiser, run over c1's output for -O.
#
# cmd/peep/GNUmakefile
#
# Built twice from the one set of sources.  The framework below builds
# it WITH ccc, FOR the Z80, which is the pass that runs on micronix and
# the one the driver execs out of /libexec.  host.mk at the foot builds
# the same sources with gcc -m32, to run here.
#
# Not in cmd/GNUmakefile's DIRS: a pass is not a command.  It installs
# into /libexec and nobody types it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= peep
CLASS	= libexec

#
# Not /bin.  ccc runs this; nobody types it.
#
BINDIR	= /libexec

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
#
HOSTDEFS =
HOSTCFLAGS = -m32 $(HOSTDEBUG) $(HOSTWARNS) -I. -I$(DEPTH)/lib/libccc
HOSTPROG = mxpeep

include $(DEPTH)/libexec/host.mk
