#
# native Makefile for whitesmiths offbeat library
#
# note that due to version madness, this generated library
# is named libWS.a, distinct from libws.a, which is the micronix
# distributed version that has a slightly but importantly different
# std.h FIO structure
#
# src/cmd/whitesmith/lib/libws/Makefile
#
# Changed: <2023-07-28 19:18:52 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE=-I/usr/src/cmd/whitesmith/include

LIB = libWS.a

all: $(LIB)

install: $(LIB)
	cp $(LIB) /lib

BROKEN = alloc.o getc.o getl.o match.o mkord.o pat.o
#
# command line limit prevents these from being on same command
#
LIBOBJS1 = $(BROKEN) \
	btoi.o btol.o btos.o buybuf.o cgetf.o cmpbuf.o \
	cmpstr.o cputf.o cpybuf.o cpystr.o cround.o crypt.o decode.o \
	doesc.o encode.o enter.o errfmt.o error.o fclose.o fcreat.o

LIBOBJS2 = \
	fill.o finit.o fioerr.o fopen.o fread.o frelst.o fwrite.o \
	gbfile.o getch.o getf.o getfi.o getfl.o getfmt.o \
	getin.o getlin.o inbuf.o instr.o itob.o itols.o

LIBOBJS3 = \
	lenstr.o lower.o lstoi.o lstol.o lstou.o ltob.o ltols.o \
	mapchr.o notbuf.o notstr.o ordbuf.o \
	pathnm.o pname.o prefix.o putc.o putch.o putf.o putfmt.o

LIBOBJS4 = \
	putl.o putlin.o putstr.o readerr.o remark.o scnbuf.o scnstr.o \
	sort.o sqze.o stdin.o stdout.o stob.o subbuf.o substr.o \
	usage.o writerr.o

LIBOBJS = $(LIBOBJS1) $(LIBOBJS2) $(LIBOBJS3) $(LIBOBJS4)

#
# the builtin echo to the shell is busted for redirection
#
#	obj $(LIBOBJS1) > obj1
#	obj $(LIBOBJS2) > obj2
#	cat obj1 obj2 > objs 
#	lord -s >objlist <objs
#
# broken out because of command line limitations
#
$(LIB): $(LIBOBJS) Makefile
	rm -f $(LIB)
	obj $(LIBOBJS1) > objs
	obj $(LIBOBJS2) >> objs
	obj $(LIBOBJS3) >> objs
	obj $(LIBOBJS4) >> objs
	lord -s > objlist < objs
	/bin/echo 1,-+s/%/lib $(LIB) -r / > editscript
	/bin/echo w >> editscript
	/bin/echo q >> editscript
	/bin/echo "cat editscript | edit objlist" > makelib.sh
	/bin/echo "sh -v objlist" >> makelib.sh
	sh -v makelib.sh
	rm -f objs editscript makelib.sh objlist

$(LIBOBJS) : $*.c
	cc $(INCLUDE) -c $*.c

clean:
	rm -f *.o objs editscript makelib.sh objlist

clobber: clean
	rm -f $(LIB)

