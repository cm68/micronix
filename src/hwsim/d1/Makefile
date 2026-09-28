#
# Makefile for morrow decision 1 hardware simulator
#
# hwsim/d1/Makefile
#
# Changed: <2023-07-10 14:47:50 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I../../include -I..
LDFLAGS = -L../../lib -lsim$(LIBSUFFIX) -lcurses

# The normal build is d1 and the -DNODEBUG build is d1p - the performance
# simulator d1 execs when it is run with -P.  OBJSUFFIX keeps the two
# builds' objects apart (djdma.o beside djdma-nodbg.o) so both binaries
# can be linked and left in place.
ifdef NODEBUG
TARGET = d1p
else
TARGET = d1
endif
TARGETS = $(TARGET)
DRIVERS = djdma$(OBJSUFFIX).o multio$(OBJSUFFIX).o hddma$(OBJSUFFIX).o \
	hdca$(OBJSUFFIX).o ide$(OBJSUFFIX).o ncr5380$(OBJSUFFIX).o \
	mpz80$(OBJSUFFIX).o micronix$(OBJSUFFIX).o kdump$(OBJSUFFIX).o
GENERATED = roms/mon447.bin roms/mon447.sym roms/mon375.bin roms/mon375.sym \
	roms/mon500.bin roms/mon500.sym
DIRS = roms
SIM = ../hwsim$(OBJSUFFIX).o ../s100$(OBJSUFFIX).o

# The libraries are prerequisites, not just link arguments.  Without
# them here a change to libsim.a - the disassembler, the monitor, the
# syscall formatter - relinks nothing, and the simulator keeps running
# the code you just replaced while you wonder why the fix did nothing.
LIBS = ../../lib/libsim$(LIBSUFFIX).a

include ../../Makefile.inc

$(TARGET): $(DRIVERS) $(GENERATED) $(SIM) $(LIBS)
	$(CC) $(DEBUG) $(GCCFLAGS) -o $(TARGET) $(DRIVERS) $(SIM) $(LDFLAGS)
	-pkill d1

$(GENERATED):   
