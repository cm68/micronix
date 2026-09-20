#
# makefile for rmdir
#
# micronix/cmd/rmdir/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= rmdir.1
MANDIR	= /usr/man/man1
PROG	= rmdir
OBJS	= rmdir.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/rmdir
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
