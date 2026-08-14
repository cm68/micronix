#
# cross build for libccc
#
# lib/libccc/GNUmakefile
#
# The compiler's own runtime: fmtlong and libutil, which every pass
# calls and none of them contains.  cpp, c0 and c1 compiled here long
# before this directory existed and then failed to link on _fmtlong
# and _fmtstr, because there was nothing in the tree to link them
# against.
#
# The sources are symlinks into ~/src/ccc/src/ccclib, where they are
# edited, the same way libexec/cpp and libexec/c0 mirror src/cpp and
# src/pass1.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= libccc
CLASS	= lib
TARGET	= libccc.a

include $(DEPTH)/GNUmakefile.inc
