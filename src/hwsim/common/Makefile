EXTRAS = sample_shim.o sample_cpu.o terminal
OBJECTS = sim.o imdlib.o dis.o s100.o terminal.o harddisk.o
TOOLS = imd

TARGETS = $(TOOLS) $(EXTRAS) libsim.a

include ../Makefile.inc

libsim.a:	$(OBJECTS)

terminal: terminal.c sim.h
	$(CC) $(CFLAGS) -DSTAND -o terminal terminal.c

imd: imd.c imd.h imdlib.o util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o imdlib.o
