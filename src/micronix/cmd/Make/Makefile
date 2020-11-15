#
# makefile for building make on linux

CFLAGS = -fdiagnostics-color=always -g

# first define some macros we will be using

MFILES	 = make.c makeinit.c makeread.c maketime.c ../../lib/libutil/hexdump.c
OBJECTS = make.o makeinit.o makeread.o maketime.o hexdump.o

TFILES	 = touch.c
TOBJECTS = touch.o

Make : $(OBJECTS)
	cc $(CFLAGS) -o Make $(OBJECTS)

clean :
	rm -f $(OBJECTS)

clobber : Makefile
	rm -f $(OBJECTS) Make

hexdump.o: ../../lib/libutil/hexdump.c
	cc -c ../../lib/libutil/hexdump.c
