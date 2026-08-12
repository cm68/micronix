#
# makefile for vi
#
# micronix/cmd/vi/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same program on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
BIN	= /bin
PROG	= vi
HDRS	= stevie.h
LIBS	= -lutil
OBJS	= cmdline.o edit.o linefunc.o main.o misccmds.o normal.o \
	window.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

#
# One entry for every object: $* is the target without its suffix, so
# this reads as "each of these comes from the .c of the same name".
#
$(OBJS): $*.c $(HDRS)
	$(CC) $(CFLAGS) -c $*.c

install: $(PROG)
	cp $(PROG) $(BIN)/vi

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(PROG)
