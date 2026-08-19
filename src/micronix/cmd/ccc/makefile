#
# makefile for ccc
#
# micronix/cmd/ccc/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# The driver.  This one IS typed, so it goes into /bin - it is the
# only part of the compiler that does.  It finds its passes from
# its own path, as bin/../libexec.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# -DCCC: this is the driver that runs ON micronix, so the linker it
# reaches for is /libexec/ld and not /libexec/mxld.  The GNUmakefile
# beside this one has the #ifdef it switches, and why a driver built
# without it goes looking for a program named after the other machine.
#
CC	= ccc
CFLAGS	= -DCCC
BIN	= /bin
MAN	= ccc.1
MANDIR	= /usr/man/man1
PROG	= ccc
OBJS	= ccc.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	cp $(PROG) $(BIN)/ccc
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
