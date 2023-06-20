# simulator library makefile
#
# lib/Makefile
#
# Changed: <2023-06-20 12:10:31 curt>

CHIPS = libchips.a
CHIPSOBJS = z80_shim.o

MEM = libmem.a
MEMOBJS = mem.o

EMU = libemu.a
EMUOBJS = emushim.o z80emu.o

OBJECTS = util.o imdlib.o openx.o disz80.o \
	mnix_sys.o whitesmith.o fslib.o gui.o

LIBRARY = libsim.a

GENERATED = tables.h

TARGETS = $(LIBRARY) $(CHIPS) $(EMU) $(MEM) maketables

INCLUDE = -I../include

include ../Makefile.inc

$(LIBRARY): $(OBJECTS)
	ar rv $(LIBRARY) $(OBJECTS)

$(CHIPS): $(CHIPSOBJS)
	ar rv $(CHIPS) $(CHIPSOBJS)

$(EMU): $(EMUOBJS)
	ar rv $(EMU) $(EMUOBJS)

$(MEM): $(MEMOBJS)
	ar rv $(MEM) $(MEMOBJS)

maketables: maketables.c
	$(CC) -Wall $< -o maketables

tables.h: maketables
	./maketables > $@

emushim.o: emushim.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

mem.o: mem.c

