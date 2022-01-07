# Makefile for user mode simulator
#
# usersim/Makefile
# Changed: <2022-01-05 13:56:26 curt>
#

CHIPS=1

ifdef CHIPS
EMU=../hwsim/chips/shim.o mem.o
else
EMU = z80emu.o z80shim.o
endif
LDFLAGS = -L../lib -lsim
LDFLAGS = -L../lib -lsim
OBJECTS = micronix.o $(EMU)
TARGETS = sim

include ../Makefile.inc

sim: $(OBJECTS) ../lib/libsim.a
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80shim.o: z80shim.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

micronix.o: micronix.c z80emu.h z80config.h

mem.o: mem.c

test: sim
	./sim -d ../../filesystem ../../filesystem/bin/ls

