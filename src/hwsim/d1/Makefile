Z80SIM = ../chips
Z80LIB = -L$(Z80SIM) -lz80
SIMLIBS = $(Z80SIM)/libz80.a ../../lib/libsim.a ../common/libhwsim.a

TARGETS=d1
DRIVERS=djdma.o multio.o hddma.o hdca.o mpz80.o # micronix.o
GENERATED = mon447.bin mon447.sym mon375.bin mon375.sym

include ../Makefile.inc

d1: $(DRIVERS) $(SIMLIBS) $(GENERATED)
	$(CC) $(DEBUG) -o d1 $(DRIVERS) $(SIM) -L ../../lib -L../common -lhwsim -lsim $(Z80LIB) 
	-pkill d1

$(GENERATED):
	cd roms ; make install

clobber::
	cd roms ; make clobber
