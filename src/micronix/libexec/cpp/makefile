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
# man page.  The sources are symlinks into ~/src/ccc, which is where
# they are edited, and the object list is spelled out because this
# make has no wildcard.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
LIBEXEC	= /libexec
PROG	= cpp
OBJS	= cfold.o cpp.o emit.o filtutil.o io.o knr.o kw.o kwtab.o \
	lex.o lexdata.o macro.o norm.o tdsrc.o util.o

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
	cp $(PROG) $(LIBEXEC)/pass0

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
