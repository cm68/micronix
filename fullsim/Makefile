CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG)

DRIVERS=djdma.o imd.o multio.o hddma.o hdca.o
CPU=mpz80.o

all: hsim imd imdd

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h
	$(CC) $(CFLAGS) -c $<

util.o:	util.c sim.h
	$(CC) $(CFLAGS) -c $<

mpz80.o: mpz80.c
	$(CC) $(CFLAGS) -c $<

hsim.o: hsim.c z80emu.h z80config.h sim.h
	$(CC) $(CFLAGS) -c $<

multio.o: multio.c sim.h
	$(CC) $(CFLAGS) -c $<

djdma.o: djdma.c sim.h
	$(CC) $(CFLAGS) -c $<

hddma.o: hddma.c sim.h
	$(CC) $(CFLAGS) -c $<

hdca.o: hdca.c sim.h
	$(CC) $(CFLAGS) -c $<

imd: imd.c sim.h util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o

imdd: imdd.c
	$(CC) -DSTAND $(CFLAGS) -o imdd imdd.c

hsim: hsim.o $(CPU) z80emu.o util.o $(DRIVERS) ../src/sgs/z80dis.o
	$(CC) $(DEBUG) -o hsim hsim.o $(CPU) z80emu.o util.o $(DRIVERS) ../src/sgs/z80dis.o
	pkill hsim

rmmerge:
	rm -f *-merge 

rmdelta:
	rm -f *-delta 

clean: rmdelta rmmerge
	rm -f *.o maketables hsim dumpfile logfile file.inp

clobber:	clean
	rm -f sim imd imdd hdca-[0-3] hddma-[0-3]
