#
# makefile for asz
#
# micronix/cmd/asz/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The assembler.  ccc runs it as the pass after c1, so it goes into
# /libexec beside the others.
#
# The object list is spelled out because this make has no wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
LIBEXEC	= /libexec
PROG	= asz
HDRS	= asm.h wsobj.h
OBJS	= asm.o asmz80.o asz.o wsobj.o

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
	cp $(PROG) $(LIBEXEC)/asz

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
