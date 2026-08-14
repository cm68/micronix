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

include $(DEPTH)/GNUmakefile.inc
