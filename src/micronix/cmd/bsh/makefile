#
# makefile for bsh
#
# micronix/cmd/bsh/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
MAN	= bsh.1 sh.1
MANDIR	= /usr/man/man1
PROG	= bsh
HDRS	= brkincr.h ctype.h defs.h dup.h mac.h mode.h name.h stak.h \
	  sym.h timeout.h
OBJS	= argscan.o args.o blok.o builtin.o cmd.o ctype.o edit.o error.o expand.o \
	  fault.o globals.o history.o io.o macro.o main.o msg.o name.o \
	  print.o service.o setbrk.o shim.o stak.o string.o word.o xec.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $(HDRS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)
	cp $(PROG) $(BIN)/sh
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)

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
	cmp $(PROG) $(BIN)/$(PROG)
	cmp $(PROG) $(BIN)/sh
	cmp bsh.1 $(MANDIR)/bsh.1
	cmp sh.1 $(MANDIR)/sh.1
