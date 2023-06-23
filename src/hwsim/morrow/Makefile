#
# Makefile for morrow
#
#
#
CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -I../common -fdiagnostics-color=always
Z80SIM = ../chips
Z80LIB = -L$(Z80SIM) -lz80
SIMLIBS = $(Z80SIM)/libz80.a ../common/libsim.a
NOUI = ../common/sim.o
UI = ../common/uisim.o

TARGETS=d1 d1-ui
DRIVERS=djdma.o multio.o hddma.o hdca.o mpz80.o micronix.o 8259.o 8250.o 1990.o

all: $(TARGETS)

d1-ui: $(DRIVERS) $(SIMLIBS) $(UI)
	$(CC) $(DEBUG) -o $@ $(DRIVERS) $(UI) -L../common -lsim $(Z80LIB) 
	-pkill $@

d1: $(DRIVERS) $(SIMLIBS) $(NOUI)
	$(CC) $(DEBUG) -o d1 $(DRIVERS) $(NOUI) -L../common -lsim $(Z80LIB) 
	-pkill d1

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out

clobber:	clean
	rm -f $(TARGETS)

# vim: tabstop=4 shiftwidth=4 expandtab:

