#
# makefile for wc
#
# micronix/cmd/wc/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
BIN	= /bin
MAN	= wc.1 chars.1 lines.1 words.1
MANDIR	= /usr/man/man1
PROG	= wc
OBJS	= wc.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
#
# chars, lines and words are this same binary under three other names,
# the way help is man's; main() tells them apart by looking at argv[0].
# Links and not copies, which is what cmd/ed/makefile does for e.
#
	ln $(BIN)/$(PROG) $(BIN)/chars
	ln $(BIN)/$(PROG) $(BIN)/lines
	ln $(BIN)/$(PROG) $(BIN)/words
	mkdir -p $(MANDIR)
#
# $(MANDIR) and not $(MANDIR)/$(MAN): cp takes one destination, and
# there are four pages now where there used to be one.
#
	cp $(MAN) $(MANDIR)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
