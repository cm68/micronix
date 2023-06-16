#
# Makefile for all hardware simulators and tools
#
# hwsim/common/Makefile
#
# Changed: <2023-06-16 00:11:53 curt>
#
EXTRAS = sample_shim.o sample_cpu.o
OBJECTS = sim.o s100.o

TARGETS = $(EXTRAS) libhwsim.a

include ../Makefile.inc

libhwsim.a:	$(OBJECTS)
	$(AR) -rv $@ $?

terminal: terminal.c sim.h
	$(CC) $(CFLAGS) -DSTAND -o terminal terminal.c

imd: imd.c imd.h imdlib.o util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o imdlib.o
