#
# native Makefile for whitesmiths wslib
#
# src/cmd/whitesmith/wslib/Makefile
#
# Changed: <2023-07-10 21:06:51 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: libws.a

BROKEN = alloc.o getc.o getl.o match.o mkord.o pat.o
#
# command line limit prevents these from being on same command
#
LIBOBJS1 = \
	btoi.o btol.o btos.o buybuf.o cgetf.o cmpbuf.o \
	cmpstr.o cputf.o cpybuf.o cpystr.o cround.o crypt.o decode.o \
	doesc.o encode.o enter.o errfmt.o error.o fclose.o fcreat.o \
	fill.o finit.o fioerr.o fopen.o fread.o frelst.o fwrite.o \
	gbfile.o getch.o getf.o getfi.o getfl.o getfmt.o
LIBOBJS2 = \
	getin.o getlin.o inbuf.o instr.o itob.o itols.o \
	lenstr.o lower.o lstoi.o lstol.o lstou.o ltob.o ltols.o \
	mapchr.o notbuf.o notstr.o ordbuf.o \
	pathnm.o pname.o prefix.o putc.o putch.o putf.o putfmt.o \
	putl.o putlin.o putstr.o readerr.o remark.o scnbuf.o scnstr.o \
	sort.o sqze.o stdin.o stdout.o stob.o subbuf.o substr.o \
	usage.o writerr.o

libws.a: $(LIBOBJS1) $(LIBOBJS2)
	lib libws.a -cv $(LIBOBJS1)
	lib libws.a -rv $(LIBOBJS2)

$(LIBOBJS) : $*.c

	cc -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f pp

