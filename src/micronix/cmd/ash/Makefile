#
# Makefile for sh
#
# micronix/cmd/ash/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

OBJ	= \
	sh1.o \
	sh2.o \
	sh3.o \
	sh5.o \
	eval.o word.o glob.o

all:	sh

sh:	$(OBJ)
	cc -o sh $(OBJ)

$(OBJ):	sh.h $*.c
	cc -c $*.c

clean:
	rm -f sh *.o *.bak core

# vim: tabstop=4 shiftwidth=4 expandtab:
