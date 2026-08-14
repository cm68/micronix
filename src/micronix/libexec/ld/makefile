#
# makefile for ld
#
# micronix/libexec/ld/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The link editor.  This used to say that a person types it as well as
# ccc running it, so it goes on the path - and it lived in cmd and
# installed into /bin.  It is in libexec now because the driver says
# so: ccc.c builds the path as libexecdir/ld, and libexecdir is
# bin/../libexec, so a linker in /bin is a linker ccc cannot find.
# The comment there is that the passes, the assembler and the linker
# are programs other programs run and do not belong on a search path.
# Typing it still works, spelled /libexec/ld.
#
# The object list is spelled out because this make has no wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
LIBEXEC	= /libexec
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
	cp $(PROG) $(LIBEXEC)/ld

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
