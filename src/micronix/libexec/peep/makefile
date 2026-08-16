#
# makefile for peep
#
# micronix/cmd/peep/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# peep is a PASS and not a command.  ccc runs it, nobody types it, so
# it goes into /libexec beside the other passes rather than /bin -
# which is what libexec has always been for, programs run by other
# programs.  The driver works out where they are from its own path,
# bin/../libexec, so the two have to stay together.  /lib is for
# things to link against and keeps none of these.  That is also why
# there is no man page here: nothing invokes it by name.
#
# The sources are copies, forked from ~/src/ccc and edited here.  Building this on micronix is the point of the exercise -
# the compiler compiling its own passes - so the object list is
# spelled out rather than wildcarded, because this make has no
# wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
LIBEXEC	= /libexec
PROG	= peep
HDRS	= peep.h
OBJS	= peep.o pool.o regs.o rules.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c $(HDRS)
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(LIBEXEC)/$(PROG)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
