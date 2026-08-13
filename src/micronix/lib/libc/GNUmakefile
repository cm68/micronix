#
# libc - the one C library
#
# lib/libc/GNUmakefile
#
# THE CROSS BUILD.  The compiler, its flags and its include path come
# from GNUmakefile.inc at the top of the tree; this file says only
# what is particular to libc.
#
# A .c and a .s of the same name are NOT a mistake.  The .c is the
# reference implementation, the .s is a hand written one defining the
# same symbol, and the .s wins.  The .c stays in the tree and is
# listed below, so that it reads as deliberate rather than forgotten.
#
# The lists are explicit rather than globbed.  Globbing loses the
# reasons: the float files cannot be assembled at all (they use deff,
# which went away because there was never one Z80 float format), and
# the math sources want a double type the compiler does not have.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

DEPTH	= ../..
DIR	= libc
CLASS	= lib
TARGET	= libc.a

ASSRCS = \
	abs.s allsh.s alrsh.s asallsh.s asalrsh.s asar.s \
	strdup.s exit.s asdiv.s asladd.s asland.s asll.s \
	asllrsh.s aslmul.s aslor.s aslr.s aslsub.s aslxor.s \
	asmod.s asmul.s atoi.s bitfield.s blkclr.s bmove.s \
	csv.s brelop.s wrelop.s getsp.s idiv.s imul.s \
	index.s inout.s iregset.s isalpha.s isdigit.s islower.s \
	isspace.s isupper.s ladd.s land.s lcom.s ldiv.s \
	linc.s lldst.s llrsh.s lmul.s longjmp.s lor.s \
	lrelop.s lsub.s lushr.s lxor.s max.s swap.s \
	qadd.s qsub.s qand.s qor.s qxor.s qcom.s \
	qcmp.s qshl.s qsar.s qshr.s qinc.s qldst.s \
	qmul.s qdiv.s op16.s tramp.s rcsv.s rindex.s \
	sbrk.s shar.s shl16.s shll.s shlr.s strcat.s \
	strchr.s strcmp.s strcpy.s swdisp.s swdispw.s oarg.s strlen.s \
	strncat.s strncmp.s strncpy.s strrchr.s tolower.s toupper.s \
	xtoi.s fputc.s fgetc.s

CSRCS = \
	rand.c srand1.c mkstemp.c mktemp.c isatty.c permaloc.c timezone.c ctime.c \
	assert.c calloc.c malloc.c memcmp.c memcpy.c memset.c \
	blkcpy.c ctype_.c cleanup.c fclose.c fflush.c filbuf.c \
	flsbuf.c puts.c fopen.c fread.c freopen.c fwrite.c \
	setbuf.c gets.c ungetc.c buf.c getenv.c pnum.c \
	qsort.c remove.c rewind.c fseek.c ftell.c fprintf.c \
	sprintf.c printf.c doprnt.c putchar.c atol.c fscanf.c \
	scanf.c sscanf.c doscan.c fnum.c atof.c mkdir.c \
	opendir.c system.c

# superseded by the .s beside them: kept, not built
NOTBUILT_C = \
	acos.c asin.c atan.c atan2.c atoi.c ceil.c \
	cos.c cosh.c evalpoly.c exp.c fabs.c floor.c \
	index.c log.c rindex.c sin.c sinh.c sqrt.c \
	strdup.c strncmp.c tan.c tanh.c

# cannot be assembled - deff no longer exists
NOTBUILT_S = \
	asfloat.s bios.s doprnt_ws.s fbcd.s finc.s float.s \
	frelop.s frexp.s frndint.s ftol.s imul816.s ltof.s \
	srand.s start1.s start2.s

# held out: ccc's longjmp.s already defines _setjmp, and which file
# should own it is still being decided.
HELD = setjmp.s

include $(DEPTH)/GNUmakefile.inc
