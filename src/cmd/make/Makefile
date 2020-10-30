# makefile for building 'make' and 'touch'

CFLAGS = -fdiagnostics-color=always -g

# first define some macros we will be using

MFILES	 = make.c makeinit.c makeread.c maketime.c
OBJECTS = make.o makeinit.o makeread.o maketime.o

TFILES	 = touch.c
TOBJECTS = touch.o

Make : $(OBJECTS)
	cc $(CFLAGS) -o Make $(OBJECTS)

clean :
	rm -f $(OBJECTS)

clobber : Makefile
	rm -f $(OBJECTS) Make
