#
# cross build for makekey
#
# cmd/makekey/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into makekey.
#
# BINDIR IS NOT THE DEFAULT HERE, and it is the whole point of this
# makefile.  crypt (1) forks the literal path /usr/lib/makekey, so
# that is where the installed program has to land; /bin is where
# GNUmakefile.inc puts everything else, and a makekey left there is a
# makekey crypt cannot find.  LIBDIR - /lib - is the library
# directory ccc reads libc.a out of, and is a different thing again.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= makekey
CLASS	= cmd
BINDIR	= /usr/lib

include $(DEPTH)/GNUmakefile.inc
