CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG)

OBJECTS = util.o sim.o imdlib.o dis.o s100.o simple.o
TOOLS = imd

all: $(OBJECTS) $(TOOLS)

simple.o: simple.c
	$(CC) $(CFLAGS) -c simple.c

dis.o: dis.c dis.h
	$(CC) $(CFLAGS) -c dis.c

imdlib.o: imd.h util.h
	$(CC) $(CFLAGS) -c imdlib.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

s100.o: s100.c sim.h
	$(CC) $(CFLAGS) -c s100.c

sim.o: sim.c sim.h util.h imd.h dis.h
	$(CC) $(CFLAGS) -c sim.c

imd: imd.c imd.h imdlib.o util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o imdlib.o

clean:
	rm -f *.o 

clobber:	clean
	rm -f $(OBJECTS) $(TOOLS) make.out
