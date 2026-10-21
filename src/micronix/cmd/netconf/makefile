#
# makefile for netconf
#
# micronix/cmd/netconf/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= netconf.1
MANDIR	= /usr/man/man1
PROG	= netconf
OBJS	= netconf.o

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

#
# verify - the inverse of install: the just-built artifact against the
# copy the first generation installed.
#
verify: $(PROG)
	cmp $(PROG) $(BIN)/$(PROG)
	cmp $(MAN) $(MANDIR)/$(MAN)
