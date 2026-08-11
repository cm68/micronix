#
# cross build for the kernel
#
# sys/GNUmakefile
#
# "all" does not build a kernel yet, and does not pretend to.  The
# kernel is not a program ccc can drive to completion: Makefile beside
# this one links three overlays at a fixed text base
#
#	link -tb0x1000 -r -o unix low.o io.o -lm.a high.o
#
# and compiles two of the files with overlay numbers (-x0, -x7).  ccc
# has no -tb, no -r and no -x, so the link has to be done by calling
# wsld directly, with the segment layout worked out.  That is its own
# piece of work; until it is done this builds the objects, which is
# the part that can be checked, and says so rather than reporting a
# kernel it did not produce.
#
# update, clean and clobber are real.  update used to be
#
#	update:
#		(cd .. ; make update)
#
# which walked UP the tree - harmless only because the top level's
# update did nothing at the time.  Now that the top level recurses
# into sys, that would have been an infinite loop.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ..
DIR	= sys
CLASS	=
SRCDIR	= /usr/src/sys

# no ccc driver can link this yet - see above
TARGET	=

# the kernel includes its own headers, not the C library's
INC	= -I$(DEPTH)/include

include $(DEPTH)/GNUmakefile.inc

#
# what "all" would produce if the overlay link existed
#
objects: $(OBJS)
