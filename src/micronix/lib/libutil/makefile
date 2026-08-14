#
# makefile for libutil
#
# micronix/lib/libutil/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same library on the host, and the object lists
# here are that file's ASSRCS and CSRCS - the two compile the same
# files, so a difference between them is a real one.
#
# ccc drives the assembler as well as the compiler, so an .s and a .c
# are the same command, which is what the cross build does too.  The
# Whitesmith cc, its as and its lib are not used and neither is
# /lib/libS.a: that compiler returns in BC where this one returns in
# HL, so its library cannot be linked against this one's code.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
AR	= ar
LIBDIR	= /lib
TARGET	= libutil.a

COBJS	= hexdump.o logmsg.o

OBJS	= $(COBJS)

all: $(TARGET)

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(AR) cr $(TARGET) $(OBJS)

#
# One entry for each group: $* is the target without its suffix,
# so this reads as "each of these comes from the file of the same
# name".
#
$(COBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(TARGET)
	cp $(TARGET) $(LIBDIR)/$(TARGET)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(TARGET)
