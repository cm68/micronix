#
# makefile for c0
#
# micronix/libexec/c0/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The first pass: the parser and the AST writer.
#
# ccc runs this; nobody types it, so it goes into /libexec and has no
# man page.  The sources are symlinks into ~/src/ccc, which is where
# they are edited, and the object list is spelled out because this
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
CFLAGS	=
LIBEXEC	= /libexec
PROG	= c0
OBJS	= decl.o declare.o error.o eutil.o expr.o fold.o init.o \
	istream.o lexread.o name.o outast.o outfn.o outh.o parse.o \
	pass1.o pblock.o pfx.o post.o pstmt2.o regalloc.o swcnt.o \
	tparse.o type.o util.o

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
	cp $(PROG) $(LIBEXEC)/c0

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
