#
# simulator library makefile
# contains generic modules useful for any simulator
#
# lib/Makefile
#
# Changed: <2023-07-10 14:45:25 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CHIPS = libchips.a
CHIPSOBJS = z80_shim.o

EMU = libemu.a
EMUOBJS = emushim.o z80emu.o

OBJECTS = util.o imdlib.o openx.o disz80.o \
	mnix_sys.o whitesmith.o fslib.o gui.o monitor.o

LIBRARY = libsim.a

GENERATED = tables.h maketables

TARGETS = $(LIBRARY) $(CHIPS) $(EMU)

INCLUDE = -I../include

include ../Makefile.inc

$(LIBRARY): $(OBJECTS)
	$(AR) rv $(LIBRARY) $(OBJECTS)

$(CHIPS): $(CHIPSOBJS)
	$(AR) rv $(CHIPS) $(CHIPSOBJS)

$(EMU): $(EMUOBJS)
	$(AR) rv $(EMU) $(EMUOBJS)

maketables: maketables.c
	$(CC) -Wall $< -o maketables

tables.h: maketables
	./maketables > $@

emushim.o: emushim.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h
