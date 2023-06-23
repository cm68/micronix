#
# simulator library makefile
#
# lib/Makefile
#
# Changed: <2023-06-23 01:23:47 curt>

CHIPS = libchips.a
CHIPSOBJS = z80_shim.o

MEM = libmem.a
MEMOBJS = mem.o

EMU = libemu.a
EMUOBJS = emushim.o z80emu.o

S100 = libs100.a
S100OBJS = s100.o

OBJECTS = util.o imdlib.o openx.o disz80.o \
	mnix_sys.o whitesmith.o fslib.o gui.o monitor.o

LIBRARY = libsim.a

GENERATED = tables.h

TARGETS = $(LIBRARY) $(CHIPS) $(EMU) $(MEM) $(S100) maketables

INCLUDE = -I../include

include ../Makefile.inc

$(LIBRARY): $(OBJECTS)
	$(AR) rv $(LIBRARY) $(OBJECTS)

$(CHIPS): $(CHIPSOBJS)
	$(AR) rv $(CHIPS) $(CHIPSOBJS)

$(EMU): $(EMUOBJS)
	$(AR) rv $(EMU) $(EMUOBJS)

$(MEM): $(MEMOBJS)
	$(AR) rv $(MEM) $(MEMOBJS)

$(S100): $(S100OBJS)
	$(AR) rv $(S100) $(S100OBJS)

$(HWSIM): $(HWSIMOBJS)
	$(AR) rv $(HWSIM) $(HWSIMOBJS)

maketables: maketables.c
	$(CC) -Wall $< -o maketables

tables.h: maketables
	./maketables > $@

emushim.o: emushim.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h

mem.o: mem.c

# vim: tabstop=4 shiftwidth=4 expandtab:

