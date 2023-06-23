#
# Makefile for morrow decision 1 hardware simulator
#
# hwsim/d1/Makefile
#
# Changed: <2023-06-23 00:33:50 curt>

LDFLAGS = -L../../lib -lsim -lchips -lcurses

TARGETS=d1
DRIVERS=djdma.o multio.o hddma.o hdca.o mpz80.o # micronix.o
GENERATED = mon447.bin mon447.sym mon375.bin mon375.sym

include ../Makefile.inc

d1: $(DRIVERS) $(GENERATED)
	$(CC) $(DEBUG) -o d1 $(DRIVERS) $(SIM) $(LDFLAGS)
	-pkill d1

$(GENERATED):
	cd roms ; make install

clobber::
	cd roms ; make clobber

# vim: tabstop=4 shiftwidth=4 expandtab:

