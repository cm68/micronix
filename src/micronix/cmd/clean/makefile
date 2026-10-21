#
# makefile for clean
#
# micronix/cmd/clean/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# 'clean' is the program and also the make target that removes the
# build, so the binary links under the name cleanprog and installs as
# /bin/clean.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= clean.1
MANDIR	= /usr/man/man1
PROG	= cleanprog
NAME	= clean
OBJS	= clean.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(NAME)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)

#
# verify - the inverse of install: every installed artifact against
# the one this build just produced.  Run it after a clobber and a
# second build to prove the rebuild matches what the first
# generation installed.
#
verify: $(PROG)
	cmp $(PROG) $(BIN)/$(NAME)
	cmp $(MAN) $(MANDIR)/$(MAN)
