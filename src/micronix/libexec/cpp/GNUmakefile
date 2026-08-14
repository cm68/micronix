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
# The compiler's own runtime: fmtlong and libutil.  Every pass calls
# them and none of them contains them, which is what left this
# undefined at _fmtlong and _fmtstr before lib/libccc existed.
#
LIBS	= $(DEPTH)/lib/libccc/libccc.a

include $(DEPTH)/GNUmakefile.inc
