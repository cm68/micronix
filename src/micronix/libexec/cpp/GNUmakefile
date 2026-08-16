#
# cross build for cpp
#
# libexec/cpp/GNUmakefile
#
# The preprocessor.  ccc runs it as the first pass and installs it
# under the name pass0, which is what the driver has always called it
# - the file here is cpp because that is its name in ~/src/ccc, where
# these sources live and are edited.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= cpp
CLASS	= libexec

#
# Not /bin.  ccc runs this; nobody types it.
#
BINDIR	= /libexec

#
# And it installs as pass0, because that is the name the driver runs
# it by: ccc.c has sprintf(cpp_path, "%s/pass0", libexecdir), while
# every other pass is called after itself.  The native makefile beside
# this one has always copied it out under that name; the cross build
# left a /libexec/cpp nothing ever executed.
#
INSTNAME = pass0

#
# The compiler's own runtime: fmtlong and libutil.  Every pass calls
# them and none of them contains them, which is what left this
# undefined at _fmtlong and _fmtstr before lib/libccc existed.
#
LIBS	= $(DEPTH)/lib/libccc/libccc.a

#
# Spelled out, because the wildcard is wrong here in three ways: it
# would sweep in dbgtags.c, which cpp.c includes rather than compiles;
# and mkkw.c and xdump.c, which are programs of their own with their
# own main - the Z80 link said so, twice over, as duplicate _main.
#
CSRCS = cpp.c lex.c io.c macro.c kw.c emit.c util.c kwtab.c \
	lexdata.c tdsrc.c cfold.c knr.c norm.c filtutil.c

include $(DEPTH)/GNUmakefile.inc

#
# and the same sources built for this machine: the cross compiler
# everything else in the tree is compiled by.  See libexec/host.mk.
#
#
# c89 and not gnu89, and the builtin-mismatch warning off:
# cpp declares strdup itself, as it must - it has to compile with a
# compiler whose library does not.
#
HOSTWARNS = -Wall -Wno-builtin-declaration-mismatch -Werror=declaration-after-statement
HOSTCFLAGS = -m32 -std=c89 $(HOSTDEBUG) $(HOSTDEFS) $(HOSTWARNS) -I. -I$(DEPTH)/lib/libccc
HOSTSRCS = $(CSRCS)
HOSTPROG = pass0

include $(DEPTH)/libexec/host.mk
