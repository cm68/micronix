#
# makefile for libsocket
#
# micronix/lib/libsocket/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same library on the host.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	= -O
AR	= ar
LIBDIR	= /lib
TARGET	= libsocket.a

MAN	= socket.3
MANDIR	= /usr/man/man3

OBJS	= socket.o services.o

all: $(TARGET)

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(AR) cr $(TARGET) $(OBJS)

$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(TARGET)
	mkdir -p $(LIBDIR)
	cp $(TARGET) $(LIBDIR)/$(TARGET)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)

clean:
	find . -name "*.o" -print -o -name "*.nam" -print -o -name "*.tok" -print -o -name "*.x" -print | xargs rm -f

clobber: clean
	rm -f $(TARGET)
