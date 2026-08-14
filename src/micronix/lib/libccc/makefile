#
# makefile for libccc
#
# micronix/lib/libccc/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same archive on the host.
#
# The compiler's own runtime - fmtlong and libutil - which the passes
# in /libexec are linked against.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
CFLAGS	=
AR	= ar
LIBDIR	= /lib
TARGET	= libccc.a
OBJS	= libutil.o fmtlong.o

all: $(TARGET)

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(AR) cr $(TARGET) $(OBJS)

$(OBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

install: $(TARGET)
	cp $(TARGET) $(LIBDIR)/$(TARGET)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(TARGET)
