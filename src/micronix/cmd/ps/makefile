#
# makefile for ps
#
# micronix/cmd/ps/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# ps.c is the 1982 original, recovered off a floppy owned by a
# Micronix developer, and ported.  ps.c's own header comment is the
# record of the port - every place the source and this tree disagreed,
# and what was done about each.  Read it before changing anything
# here; there is no second ps.c to compare against.
#
# The man page is not here and is not copied from here.  The recovered
# 1982 page is man/man1/ps.1, which the man tree ships on its own, and
# a ps.1 beside this file would shadow it - the host man build reads
# cmd/*/*.1 before man/man1/*.1 and reports what it shadowed.  So
# install ships the binary and only the binary.
#
# One thing to know when reading a proc table by hand, because it is
# not visible from here: a ps built against today's include/sys/proc.h
# reads a 98-byte struct proc and NPROC 20.  The ps binary installed in
# filesystem/bin is an older build that reads 141 bytes and NPROC 17,
# and src/usersim/usersim.c builds that older layout by hand for it.
# The kernel still publishes the table where both of them look - see
# the "Hook for ps" in sys/uhdr.s.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
BIN	= /bin
PROG	= ps
OBJS	= ps.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
