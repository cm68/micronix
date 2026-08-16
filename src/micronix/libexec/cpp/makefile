#
# makefile for cpp
#
# micronix/libexec/cpp/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The preprocessor.  It installs as pass0 and not as cpp: that is
# the name the driver calls it by, and has been since before this
# tree had a directory for it.
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
PROG	= cpp
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
	cp $(PROG) $(LIBEXEC)/pass0

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
