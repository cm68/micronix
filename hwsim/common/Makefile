AR = ar
CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -Wno-builtin-declaration-mismatch


EXTRAS = sample_shim.o sample_cpu.o terminal
OBJECTS = util.o sim.o imdlib.o dis.o s100.o terminal.o harddisk.o
TOOLS = imd

all: $(TOOLS) libsim.a $(EXTRAS)

libsim.a:	$(OBJECTS)
	$(AR) -rv libsim.a $(OBJECTS)

sample_cpu.o: sample_cpu.c
	$(CC) $(CFLAGS) -c sample_cpu.c

dis.o: dis.c dis.h
	$(CC) $(CFLAGS) -c dis.c

harddisk.o: imd.h harddisk.c
	$(CC) $(CFLAGS) -c harddisk.c

imdlib.o: imd.h util.h
	$(CC) $(CFLAGS) -c imdlib.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

s100.o: s100.c sim.h
	$(CC) $(CFLAGS) -c s100.c

sim.o: sim.c sim.h util.h imd.h dis.h
	$(CC) $(CFLAGS) -c sim.c

terminal.o: terminal.c sim.h
	$(CC) $(CFLAGS) -c terminal.c

terminal: terminal.c sim.h
	$(CC) $(CFLAGS) -DSTAND -o terminal terminal.c

sample_shim.o: sim.h sample_shim.c
	$(CC) $(CFLAGS) -c sample_shim.c

imd: imd.c imd.h imdlib.o util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o imdlib.o

clean:
	rm -f *.o 

clobber:	clean
	rm -f libsim.a $(OBJECTS) $(EXTRAS) $(TOOLS) make.out
