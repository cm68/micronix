Z80SIM = ../chips
Z80LIB = -L$(Z80SIM) -lz80
SIMLIBS = $(Z80SIM)/libz80.a ../../lib/libsim.a ../common/libhwsim.a

TARGETS=d1
DRIVERS=djdma.o multio.o hddma.o hdca.o mpz80.o # micronix.o

include ../Makefile.inc

d1: $(DRIVERS) $(SIMLIBS) 
	$(CC) $(DEBUG) -o d1 $(DRIVERS) $(SIM) -L ../../lib -L../common -lhwsim -lsim $(Z80LIB) 
	-pkill d1

