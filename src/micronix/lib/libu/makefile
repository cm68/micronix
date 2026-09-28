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
# -x prints each pass's command as ccc runs it, so a compile that dies
# partway shows exactly which pass and how it was invoked.
#
CFLAGS	= -O -x
AR	= ar
LIBDIR	= /lib
TARGET	= libu.a

#
# The man2 pages, listed outright: cmd/make has no $(wildcard), so a
# page added here does not install until it is named in this list.
# They are the 1.3 originals off the distribution floppy.
#
MAN	= Intro.2 access.2 break.2 chdir.2 chmod.2 chown.2 \
	close.2 creat.2 csw.2 dup.2 exec.2 exit.2 fork.2 \
	fstat.2 getpid.2 getuid.2 gtty.2 indir.2 kill.2 link.2 \
	mknod.2 mount.2 nice.2 open.2 pipe.2 read.2 seek.2 \
	setuid.2 signal.2 sleep.2 stat.2 stime.2 stty.2 sync.2 \
	time.2 umount.2 unlink.2 wait.2 write.2
MANDIR	= /usr/man/man2

# The archive is built in chunks because a single "ar cr libu.a $(OBJS)"
# command line is too long for the system to exec - v6-era Micronix has
# no room for an 80-object command.  Each chunk stays comfortably under
# one argument block.
AOBJS1	= access.o alarm.o _break.o chdir.o chmod.o chown.o \
	close.o creat.o dup.o errno.o exec.o execv.o \
	exit.o _exit.o fork.o fstat.o getpid.o getuid.o \
	gtty.o kill.o link.o mknod.o mount.o nice.o

AOBJS2	= open.o pause.o perror.o pipe.o read.o reboot.o seekraw.o \
	setuid.o _signal.o sleep.o stat.o stime.o sbrk.o \
	stty.o sync.o time.o umount.o unlink.o wait.o \
	write.o ioctl.o fputc.o fgetc.o

AOBJS	= $(AOBJS1) $(AOBJS2)

COBJS	= signal.o lseek.o seek.o rename.o \
	buf.o cleanup.o fclose.o fflush.o filbuf.o flsbuf.o \
	fopen.o fread.o freopen.o fwrite.o fseek.o ftell.o \
	rewind.o setbuf.o ungetc.o gets.o puts.o putchar.o \
	printf.o fprintf.o scanf.o fscanf.o isatty.o

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
	$(AR) cr $(TARGET) $(AOBJS1)
	$(AR) r $(TARGET) $(AOBJS2)
	$(AR) r $(TARGET) $(COBJS)

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
	mkdir -p $(LIBDIR)
	cp $(TARGET) $(LIBDIR)/$(TARGET)
	cp $(CRT0) $(LIBDIR)/$(CRT0)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)

# clean removes every .o and every ccc intermediate, via find | xargs so
# the argument list never trips the shell's "too many arguments".
#
# The patterns are double-quoted on purpose.  This shell (cmd/sh) has
# no single quotes at all - "find -name '*.o'" leaves the quotes in the
# word and globs nothing, so it says "No match." and rm never runs -
# and a backslash does not protect a pattern character either.  Double
# quotes are the one thing that stops the shell expanding the *.
clean:
	find . -name "*.o" -print -o -name "*.nam" -print -o -name "*.tok" -print -o -name "*.x" -print | xargs rm -f

clobber: clean
	rm -f $(TARGET)
