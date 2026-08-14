#
# makefile for ld
#
# micronix/cmd/ld/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The link editor.  A person types this as well as ccc running it,
# so it goes on the path.
#
# The object list is spelled out because this make has no wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
BIN	= /bin
PROG	= ld
HDRS	= wsobj.h
OBJS	= ld.o wsobj.o

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
	cp $(PROG) $(BIN)/ld

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
