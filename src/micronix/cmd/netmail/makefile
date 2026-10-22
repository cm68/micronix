#
# makefile for netmail
#
# micronix/cmd/netmail/makefile
#
# The native build, inside micronix, against /bin/ccc.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
BIN	= /bin
PROG	= netmail
OBJS	= netmail.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: $(PROG)
	mkdir -p $(BIN)
	cp $(PROG) $(BIN)/$(PROG)

clean:
	rm -f $(OBJS) *.nam *.tok *.x

clobber: clean
	rm -f $(PROG)

verify: $(PROG)
	cmp $(PROG) $(BIN)/$(PROG)
