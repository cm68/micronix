#
# makefile for libc
#
# micronix/lib/libc/makefile
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
TARGET	= libc.a

#
# The man3 pages, listed outright: cmd/make has no $(wildcard), so a
# page added here does not install until it is named in this list.
#
# Two batches.  The first ten are this tree's own, written for the
# port.  The rest came off the 1.3 distribution floppy and are the
# 1982 originals.
#
# Section 3 is the library calls, and it is the section that decides
# where a page goes, not the archive the code was packed into: the
# stdio family is built into libu.a and documented here anyway,
# because fopen is a library call and the manual says so by putting
# it in section 3.  popen and ttyname are in the 1.3 manual and in
# nothing this port builds; they are here because the manual is the
# manual.  getpwent used to be named with them and is not any more -
# getpwent.c is in the archive now - so its page stays for the same
# reason as the others, and getgrent.3 was written to sit beside it.
# floor.3 sits beside a floor.c that is kept and not built, which is
# what NOTBUILT_C below is for.
#
MAN	= strcat.3 strchr.3 strcmp.3 strcpy.3 strdup.3 \
	strlen.3 strncat.3 strncmp.3 strncpy.3 strrchr.3 \
	alloc.3 ctime.3 crypt.3 fclose.3 ferror.3 floor.3 fopen.3 \
	fread.3 fseek.3 getgrent.3 getpass.3 getpwent.3 gets.3 \
	perror.3 popen.3 \
	printf.3 putc.3 puts.3 scanf.3 setbuf.3 stdio.3 \
	system.3 ttyname.3
MANDIR	= /usr/man/man3

# The archive is built in chunks: a single "ar cr libc.a $(OBJS)" line
# is too long for the system to exec (v6-era Micronix has no room for a
# 115-object command).  Each chunk stays under one argument block.
AOBJS1	= abs.o allsh.o alrsh.o asallsh.o asalrsh.o asar.o \
	strdup.o asdiv.o asladd.o asland.o asll.o \
	asllrsh.o aslmul.o aslor.o aslr.o aslsub.o aslxor.o \
	asmod.o asmul.o atoi.o bitfield.o blkclr.o bmove.o \
	csv.o brelop.o wrelop.o getsp.o idiv.o imul.o

AOBJS2	= index.o inout.o iregset.o isalpha.o isdigit.o islower.o \
	isspace.o isupper.o ladd.o land.o lcom.o ldiv.o \
	linc.o lldst.o llrsh.o lmul.o longjmp.o lor.o \
	lrelop.o lsub.o lushr.o lxor.o max.o swap.o \
	qadd.o qsub.o qand.o qor.o qxor.o qcom.o

AOBJS3	= qcmp.o qshl.o qsar.o qshr.o qinc.o qldst.o \
	qmul.o qdiv.o op16.o tramp.o rcsv.o rindex.o \
	shar.o shl16.o shll.o shlr.o strcat.o \
	strchr.o strcmp.o strcpy.o swdisp.o swdispw.o oarg.o \
	strlen.o strncat.o strncmp.o strncpy.o strrchr.o tolower.o \
	toupper.o xtoi.o memcpy.o

AOBJS	= $(AOBJS1) $(AOBJS2) $(AOBJS3)

COBJS	= rand.o srand1.o mkstemp.o mktemp.o permaloc.o \
	timezone.o ctime.o assert.o calloc.o malloc.o memcmp.o \
	memset.o blkcpy.o ctype_.o getenv.o pnum.o \
	qsort.o remove.o sprintf.o doprnt.o atol.o sscanf.o \
	doscan.o fnum.o atof.o mkdir.o opendir.o system.o \
	getpwent.o getgrent.o crypt.o getpass.o

OBJS	= $(AOBJS) $(COBJS)

all: $(TARGET)

$(TARGET): $(OBJS)
	rm -f $(TARGET)
	$(AR) cr $(TARGET) $(AOBJS1)
	$(AR) r $(TARGET) $(AOBJS2)
	$(AR) r $(TARGET) $(AOBJS3)
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

install: $(TARGET)
	mkdir -p $(LIBDIR)
	cp $(TARGET) $(LIBDIR)/$(TARGET)
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
