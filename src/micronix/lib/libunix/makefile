#
# makefile for libunix
#
# micronix/lib/libunix/makefile
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
TARGET	= libunix.a

#
# The man2 pages, listed outright: cmd/make has no $(wildcard), so a
# page added here does not install until it is named in this list.
# They are the 1.3 originals off the distribution floppy.
#
MAN	= Intro.2 access.2 break.2 chdir.2 chmod.2 chown.2 \
	close.2 creat.2 dup.2 dup2.2 exec.2 execve.2 exit.2 fork.2 \
	fstat.2 ftruncate.2 getpid.2 getuid.2 gtty.2 indir.2 ioctl.2 jobctl.2 kill.2 link.2 \
	mknod.2 mount.2 nice.2 open.2 pipe.2 read.2 seek.2 select.2 \
	setuid.2 signal.2 sleep.2 stat.2 stime.2 stty.2 sync.2 \
	time.2 times.2 umount.2 unlink.2 wait.2 write.2
MANDIR	= /usr/man/man2

# The archive is built in chunks because a single "ar cr libunix.a $(OBJS)"
# command line is too long for the system to exec - v6-era Micronix has
# no room for an 80-object command.  Each chunk stays comfortably under
# one argument block.
AOBJS1	= access.o alarm.o _break.o chdir.o chmod.o chown.o \
	close.o creat.o dup.o dup2.o errno.o exec.o execve.o execv.o \
	exit.o _exit.o fork.o fstat.o getpid.o getuid.o \
	gtty.o kill.o link.o mknod.o mount.o nice.o

AOBJS2	= sysopen.o pause.o perror.o pipe.o read.o reboot.o seekraw.o select.o \
	setuid.o _signal.o sleep.o stat.o stime.o sbrk.o \
	stty.o sync.o time.o times.o umount.o unlink.o wait.o \
	write.o ioctl.o fputc.o fgetc.o ftruncate.o

AOBJS	= $(AOBJS1) $(AOBJS2)

COBJS	= signal.o lseek.o seek.o rename.o \
	buf.o cleanup.o fclose.o fflush.o filbuf.o flsbuf.o \
	fopen.o fread.o freopen.o fwrite.o fseek.o ftell.o \
	rewind.o setbuf.o ungetc.o gets.o puts.o putchar.o \
	printf.o fprintf.o scanf.o fscanf.o isatty.o open.o

OBJS	= $(AOBJS) $(COBJS)

#
# crt0 is not in the archive.  It is the startup the linker puts in
# front of every program, named by the driver, so it is installed
# beside libunix.a rather than packed into it.
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

#
# verify - the inverse of install: every installed artifact against
# the one this build just produced.  Run it after a clobber and a
# second build to prove the rebuild matches what the first
# generation installed.
#
verify: $(TARGET) $(CRT0)
	cmp $(TARGET) $(LIBDIR)/$(TARGET)
	cmp $(CRT0) $(LIBDIR)/$(CRT0)
	cmp Intro.2 $(MANDIR)/Intro.2
	cmp access.2 $(MANDIR)/access.2
	cmp break.2 $(MANDIR)/break.2
	cmp chdir.2 $(MANDIR)/chdir.2
	cmp chmod.2 $(MANDIR)/chmod.2
	cmp chown.2 $(MANDIR)/chown.2
	cmp close.2 $(MANDIR)/close.2
	cmp creat.2 $(MANDIR)/creat.2
	cmp dup.2 $(MANDIR)/dup.2
	cmp dup2.2 $(MANDIR)/dup2.2
	cmp exec.2 $(MANDIR)/exec.2
	cmp execve.2 $(MANDIR)/execve.2
	cmp exit.2 $(MANDIR)/exit.2
	cmp fork.2 $(MANDIR)/fork.2
	cmp fstat.2 $(MANDIR)/fstat.2
	cmp ftruncate.2 $(MANDIR)/ftruncate.2
	cmp getpid.2 $(MANDIR)/getpid.2
	cmp getuid.2 $(MANDIR)/getuid.2
	cmp gtty.2 $(MANDIR)/gtty.2
	cmp indir.2 $(MANDIR)/indir.2
	cmp ioctl.2 $(MANDIR)/ioctl.2
	cmp jobctl.2 $(MANDIR)/jobctl.2
	cmp kill.2 $(MANDIR)/kill.2
	cmp link.2 $(MANDIR)/link.2
	cmp mknod.2 $(MANDIR)/mknod.2
	cmp mount.2 $(MANDIR)/mount.2
	cmp nice.2 $(MANDIR)/nice.2
	cmp open.2 $(MANDIR)/open.2
	cmp pipe.2 $(MANDIR)/pipe.2
	cmp read.2 $(MANDIR)/read.2
	cmp seek.2 $(MANDIR)/seek.2
	cmp select.2 $(MANDIR)/select.2
	cmp setuid.2 $(MANDIR)/setuid.2
	cmp signal.2 $(MANDIR)/signal.2
	cmp sleep.2 $(MANDIR)/sleep.2
	cmp stat.2 $(MANDIR)/stat.2
	cmp stime.2 $(MANDIR)/stime.2
	cmp stty.2 $(MANDIR)/stty.2
	cmp sync.2 $(MANDIR)/sync.2
	cmp time.2 $(MANDIR)/time.2
	cmp times.2 $(MANDIR)/times.2
	cmp umount.2 $(MANDIR)/umount.2
	cmp unlink.2 $(MANDIR)/unlink.2
	cmp wait.2 $(MANDIR)/wait.2
	cmp write.2 $(MANDIR)/write.2
