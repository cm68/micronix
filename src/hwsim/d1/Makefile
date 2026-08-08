#
# Makefile for morrow decision 1 hardware simulator
#
# hwsim/d1/Makefile
#
# Changed: <2023-07-10 14:47:50 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I../../include -I..
LDFLAGS = -L../../lib -lsim -lchips -lcurses

TARGETS=d1
DRIVERS=djdma.o multio.o hddma.o hdca.o mpz80.o micronix.o
GENERATED = roms/mon447.bin roms/mon447.sym roms/mon375.bin roms/mon375.sym
DIRS = roms
SIM = ../hwsim.o ../s100.o

# The libraries are prerequisites, not just link arguments.  Without
# them here a change to libsim.a - the disassembler, the monitor, the
# syscall formatter - relinks nothing, and the simulator keeps running
# the code you just replaced while you wonder why the fix did nothing.
LIBS = ../../lib/libsim.a ../../lib/libchips.a

include ../../Makefile.inc

d1: $(DRIVERS) $(GENERATED) $(SIM) $(LIBS)
	$(CC) $(DEBUG) $(GCCFLAGS) -o d1 $(DRIVERS) $(SIM) $(LDFLAGS)
	-pkill d1

$(GENERATED):   
