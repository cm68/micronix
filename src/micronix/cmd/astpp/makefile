#
# makefile for astpp
#
# micronix/cmd/astpp/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# Prints an AST.  ccc runs it for -A; nobody types it, so it goes
# into /libexec and has no man page.
#
# The object list is spelled out because this make has no wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
LIBEXEC	= /libexec
PROG	= astpp
OBJS	= astpp.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(LIBEXEC)/astpp

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
