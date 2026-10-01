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
# ps.1 beside this file is this port's man page, and it is the one that
# installs: one page per command beside its source is what the guest's
# /usr/man gets.  The recovered 1982 page is still in the tree as
# man/man1/oldps.1 - oldps, the way the 1982 sh page is oldsh.1 - so
# that the host man build ships both instead of reporting one shadowed
# by the other.  The 1982 page documents the ps this one was ported
# from; sys/attic/README records where the two diverge, and the CPU
# column it still lists is the one that is gone.
#
# One thing to know when reading a proc table by hand, because it is
# not visible from here: include/sys/proc.h's struct proc is 99 bytes
# and NPROC is 20.  The kernel publishes the table where this ps looks
# for it - see the "Hook for ps" in sys/uhdr.s.  Whoever reads that
# table without the header has to be kept in step with it by hand, and
# two of them do: src/usersim/usersim.c and hwsim/d1/kdump.c.
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
MAN	= ps.1
MANDIR	= /usr/man/man1
PROG	= ps
OBJS	= ps.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
