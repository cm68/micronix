#
# makefile for libu
#
# micronix/lib/libu/makefile
#
# The native build: this runs inside micronix, under the make in
# cmd/make, against the compiler in /bin/ccc.  The GNUmakefile beside
# it cross builds the same library on the host, and the object lists
# here are that file's ASSRCS and CSRCS - the two compile the same
# files, so a difference between them is a real one.
#
# ccc drives the assembler as well as the compiler, so an .s and a .c
# are the same command, which is what the cross build does too.  The
# Whitesmith cc, its as and its lib are not used and neither is
# /lib/libS.a: that compiler returns in BC where this one returns in
# HL, so its library cannot be linked against this one's code.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CC	= ccc
#
# -O is the peephole pass.  The cross build passes it too - see
# GNUmakefile.inc, which explains why it is not optional on a machine
# where the heap is whatever is left of 64k after the image.
#
CFLAGS	= -O
AR	= ar
LIBDIR	= /lib
TARGET	= libu.a

AOBJS	= access.o alarm.o _break.o chdir.o chmod.o chown.o \
	close.o creat.o dup.o errno.o exec.o execv.o \
	exit.o fdpos.o fork.o fstat.o getpid.o getuid.o \
	gtty.o kill.o link.o mknod.o mount.o nice.o \
	open.o pause.o perror.o pipe.o read.o seekraw.o \
	setuid.o _signal.o sleep.o stat.o stime.o sbrk.o \
	stty.o sync.o time.o umount.o unlink.o wait.o \
	write.o

COBJS	= signal.o lseek.o seek.o rename.o

OBJS	= $(AOBJS) $(COBJS)

#
# crt0 is not in the archive.  It is the startup the linker puts in
# front of every program, named by the driver, so it is installed
# beside libu.a rather than packed into it.
#
# It is built here because nothing built it anywhere.  It sat in /lib
# as a bootstrap leftover that no makefile owned, and when the first
# argument moved into HL it went on calling main by the old
# convention - argc and argv both on the stack - against a main
# compiled for the new one.  Every native build after that linked a
# program that read its own arguments as garbage, and the compiler
# that came out could not parse its own command line.  A file no rule
# mentions cannot go stale loudly; it goes stale silently.
#
CRT0	= crt0.o

all: $(TARGET) $(CRT0)

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(AR) cr $(TARGET) $(OBJS)

#
# One entry for each group: $* is the target without its suffix,
# so this reads as "each of these comes from the file of the same
# name".
#
$(AOBJS): $*.s
	$(CC) $(CFLAGS) -c $*.s

$(COBJS): $*.c
	$(CC) $(CFLAGS) -c $*.c

$(CRT0): crt0.s
	$(CC) $(CFLAGS) -c crt0.s

install: $(TARGET) $(CRT0)
	cp $(TARGET) $(LIBDIR)/$(TARGET)
	cp $(CRT0) $(LIBDIR)/$(CRT0)

clean:
	rm -f $(OBJS) $(CRT0)

clobber: clean
	rm -f $(TARGET)
