#
# makefile for s
#
# micronix/cmd/s/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
BIN	= /bin
PROG	= s
HDRS	= s.h
OBJS	= address.o adjust.o bman.o buffer.o commands.o keyboard.o \
	lib.o operator.o s.o screen.o sman.o yank.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c $(HDRS)
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/s

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
