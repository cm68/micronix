#
# makefile for pass0
#
# micronix/libexec/pass0/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# pass0 is ccc's first pass: the lexer and the normalizer.  What it
# emits is a .x stream - "compact token format" - and that is what c0
# reads.  It is not a preprocessor, and the name it was carrying, cpp,
# belonged to a different program; the driver has always run it as
# pass0, and the directory says so now too.
#
# ccc runs this; nobody types it, so it goes into /libexec and has no
# man page.  The sources are copies, forked from ~/src/ccc and edited
# here, and the object list is spelled out because this make has no
# wildcard.
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
PROG	= pass0
OBJS	= cfold.o cpp.o emit.o filtutil.o io.o knr.o kw.o kwtab.o \
	lex.o lexdata.o macro.o norm.o tdsrc.o util.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	mkdir -p $(LIBEXEC)
	cp $(PROG) $(LIBEXEC)/$(PROG)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
