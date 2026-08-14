#
# makefile for sh
#
# micronix/cmd/sh/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
BIN	= /bin
MAN	= sh.1
MANDIR	= /usr/man/man1
PROG	= sh
HDRS	= sh.h
OBJS	= sh.o parse.o glob.o hist.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c $(HDRS)
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG) install-man
	cp $(PROG) $(BIN)/$(PROG)

install-man:
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
