# Description file for the Make command
# Makefile 4.5 94/1/3
# ident.o
OBJECTS= main.o doname.o misc.o files.o dosys.o gram.o 
LIBES= 
#LINT=	lint -ps
#XSTR=   /usr/ucb/xstr
CFLAGS=	-O -I../../include/ # -I../ar
#SEPFLAG= -i
CC = zxc

.c.o:
	${CC} -c ${CFLAGS} $*.c 
# Special massaging of C files for sharing of strings
#	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
#	${CC} -c ${CFLAGS} x.c 
#	mv -f x.o $*.o
#	rm -f x.c

all:	make

make:	${OBJECTS} # strings.o
	${CC} -omake ${CFLAGS} ${SEPFLAG} ${OBJECTS} ${LIBES}

${OBJECTS}:  defs

clean:
	-rm -f *.o gram.c make a.out errs x.c xs.c strings

install: make
	install -s make ${DESTDIR}/bin/make

lint :  dosys.c doname.c files.c main.c misc.c ident.c gram.c
	${LINT} dosys.c doname.c files.c main.c misc.c ident.c gram.c
	rm -f gram.c

gram.c: gram.y
	zxcc ../yacc/yacc.com gram.y
	fgrep -v "static char yaccpar_sccsid" y.tab >gram.c
	rm -f y.tab

#
#strings.o: strings
#	${XSTR}
#	${CC} -c xs.c
#	mv -f xs.o strings.o
#	rm -f xs.c

# vim: tabstop=4 shiftwidth=4 noexpandtab:

