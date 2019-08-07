CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG)

DRIVERS=djdma.o imd.o multio.o hdcdma.o hdca.o

all: hsim

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h
	$(CC) $(CFLAGS) -c $<

decision.o: decision.c z80emu.h z80config.h sim.h
	$(CC) $(CFLAGS) -c $<

multio.o: multio.c sim.h
	$(CC) $(CFLAGS) -c $<

djdma.o: djdma.c sim.h
	$(CC) $(CFLAGS) -c $<

hdcdma.o: hdcdma.c sim.h
	$(CC) $(CFLAGS) -c $<

hdca.o: hdca.c sim.h
	$(CC) $(CFLAGS) -c $<

imd: imd.c
	$(CC) $(CFLAGS) -o imd imd.c

hsim: decision.o z80emu.o $(DRIVERS) ../src/sgs/z80dis.o
	$(CC) $(DEBUG) -o hsim decision.o z80emu.o $(DRIVERS) ../src/sgs/z80dis.o


clean:
	rm -f *.o maketables hsim

clobber:	clean
	rm -f sim
