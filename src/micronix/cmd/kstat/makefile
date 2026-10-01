#
# makefile for kstat
#
# micronix/cmd/kstat/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# kstat reads the kernel's symbol table out of /micronix and then the
# tables those symbols point at out of /dev/mem, so it dumps the live
# process table, tty structures, buffer headers, mount table, file and
# inode tables, the clist, and the device switches.
#

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= kstat.1
MANDIR	= /usr/man/man1
PROG	= kstat
OBJS	= kstat.o

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
