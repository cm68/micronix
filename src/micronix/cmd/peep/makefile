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
# it goes into /lib beside the other passes rather than /bin - the
# driver works out where they are from its own path, bin/../lib, so
# the two have to stay together.  That is also why there is no man
# page here: nothing on the system invokes it by name.
#
# The sources are symlinks into ~/src/ccc, which is where they are
# edited.  Building this on micronix is the point of the exercise -
# the compiler compiling its own passes - so the object list is
# spelled out rather than wildcarded, because this make has no
# wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
LIB	= /lib
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
	cp $(PROG) $(LIB)/$(PROG)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
