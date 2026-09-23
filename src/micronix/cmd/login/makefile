#
# makefile for login
#
# micronix/cmd/login/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass, the same as every other cmd here.
#
CFLAGS	= -O
BIN	= /bin
MAN	= login.1
MANDIR	= /usr/man/man1
PROG	= login
OBJS	= login.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/login
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)
