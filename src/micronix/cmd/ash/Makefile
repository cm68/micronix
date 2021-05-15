# Makefile for sh

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
