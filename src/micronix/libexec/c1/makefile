#
# makefile for c1
#
# micronix/libexec/c1/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The second pass: the code generator.
#
# ccc runs this; nobody types it, so it goes into /libexec and has no
# man page.  The sources are copies, forked from ~/src/ccc and edited here, and the object list is spelled out because this
# make has no wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# The compiler's own runtime, fmtlong and fmtstr.  ccc adds crt0.o,
# libc.a and libu.a out of /lib by itself and knows nothing about this
# one, so it has to be named - the GNUmakefile beside this does the
# same.  Without it the link ends at _fmtlong and _fmtstr.
#
LIBS	= /lib/libccc.a
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
LIBEXEC	= /libexec
PROG	= c1
OBJS	= astio.o expr.o lower.o parseast.o pass2.o rewrite.o rules.o ruleidx.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

#
# mkruleidx writes the rule index from the rule table - see mkruleidx.c.
# It links rules.o so it walks the very table it indexes, and refuses to
# write anything if any opcode's rules are not one unbroken run, so a
# misfiled rule stops the build instead of never being tried.  Built
# natively, with ccc, because on this machine ccc's target is this
# machine.
#
mkruleidx: mkruleidx.c rules.o
	$(CC) $(CFLAGS) -o $@ mkruleidx.c rules.o

ruleidx.c: mkruleidx rules.c
	./mkruleidx > ruleidx.c

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(LIBEXEC)/c1

clean:
	rm -f $(OBJS) mkruleidx ruleidx.c

clobber: clean
	rm -f $(PROG)
