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
MAN	= strcat.3 strchr.3 strcmp.3 strcpy.3 strdup.3 \
	strlen.3 strncat.3 strncmp.3 strncpy.3 strrchr.3
MANDIR	= /usr/man/man3

AOBJS	= abs.o allsh.o alrsh.o asallsh.o asalrsh.o asar.o \
	strdup.o asdiv.o asladd.o asland.o asll.o \
	asllrsh.o aslmul.o aslor.o aslr.o aslsub.o aslxor.o \
	asmod.o asmul.o atoi.o bitfield.o blkclr.o bmove.o \
	csv.o brelop.o wrelop.o getsp.o idiv.o imul.o \
	index.o inout.o iregset.o isalpha.o isdigit.o islower.o \
	isspace.o isupper.o ladd.o land.o lcom.o ldiv.o \
	linc.o lldst.o llrsh.o lmul.o longjmp.o lor.o \
	lrelop.o lsub.o lushr.o lxor.o max.o swap.o \
	qadd.o qsub.o qand.o qor.o qxor.o qcom.o \
	qcmp.o qshl.o qsar.o qshr.o qinc.o qldst.o \
	qmul.o qdiv.o op16.o tramp.o rcsv.o rindex.o \
	shar.o shl16.o shll.o shlr.o strcat.o \
	strchr.o strcmp.o strcpy.o swdisp.o swdispw.o oarg.o \
	strlen.o strncat.o strncmp.o strncpy.o strrchr.o tolower.o \
	toupper.o xtoi.o

COBJS	= rand.o srand1.o mkstemp.o mktemp.o permaloc.o \
	timezone.o ctime.o assert.o calloc.o malloc.o memcmp.o \
	memcpy.o memset.o blkcpy.o ctype_.o getenv.o pnum.o \
	qsort.o remove.o sprintf.o doprnt.o atol.o sscanf.o \
	doscan.o fnum.o atof.o mkdir.o opendir.o system.o

OBJS	= $(AOBJS) $(COBJS)

all: $(TARGET)

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

install: $(TARGET)
	cp $(TARGET) $(LIBDIR)/$(TARGET)
	cp $(MAN) $(MANDIR)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(TARGET)
