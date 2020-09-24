INCLUDE = -I../../include -I../include

OBJECTS = z80emu.o micronix.o
TARGETS = sim
LDFLAGS = -L../lib -lsim

include ../Makefile.inc

sim: $(OBJECTS) ../lib/libsim.a
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

micronix.o: micronix.c z80emu.h z80config.h

test: sim
	./sim -d ../../filesystem ../../filesystem/bin/ls

