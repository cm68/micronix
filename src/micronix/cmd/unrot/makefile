#
# makefile for unrot
#
# micronix/cmd/unrot/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /usr/bin
MAN	= unrot.1
MANDIR	= /usr/man/man1
PROG	= unrot
OBJS	= unrot.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
