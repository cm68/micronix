#
# makefile for awk
#
# micronix/cmd/awk/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# lex.yy.c, y.tab.c and proctab.c are generated files, committed so the
# native build does not have to run lex/yacc/proc first.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
PROG	= awk
OBJS	= b.o lib.o main.o parse.o run.o token.o tran.o freeze.o popen.o \
	  intmath.o y.tab.o lex.yy.o proctab.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/$(PROG)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
