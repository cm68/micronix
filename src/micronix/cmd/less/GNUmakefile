#
# cross build for less
#
# cmd/less/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into less.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= less
CLASS	= cmd

# less.help rides the source tar so the native install (makefile:45)
# can copy it to /usr/lib; it matches none of the *.c/*.h/*.s/*.1
# wildcards in GNUmakefile.inc.
EXTRA	= less.help


HSRCS	= funcs.h less.h position.h
MANSRCS	= less.1

CSRCS	= main.c option.c prim.c ch.c position.c input.c output.c screen.c \
	 prompt.c line.c signal.c help.c ttyin.c command.c version.c

include $(DEPTH)/GNUmakefile.inc
