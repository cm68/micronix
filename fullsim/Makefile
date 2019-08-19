CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG)

CP_DRIVERS=ss1.o disk1.o
D1_DRIVERS=djdma.o multio.o hddma.o hdca.o

D1_CPU=mpz80.o
CP_CPU=cpuz.o

all: d1 imd imdd # cpro

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

ss1.o: ss1.c sim.h
	$(CC) $(CFLAGS) -c $<

disk1.o: disk1.c sim.h
	$(CC) $(CFLAGS) -c $<

cpuz.o: cpuz.c sim.h
	$(CC) $(CFLAGS) -c $<

imd: imd.c sim.h util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o

imdd: imdd.c
	$(CC) -DSTAND $(CFLAGS) -o imdd imdd.c

d1: hsim.o $(D1_CPU) z80emu.o util.o imd.o $(D1_DRIVERS) ../src/sgs/z80dis.o
	$(CC) $(DEBUG) -o d1 hsim.o $(D1_CPU) imd.o z80emu.o util.o $(D1_DRIVERS) ../src/sgs/z80dis.o
	pkill d1

cpro: hsim.o $(CP_CPU) z80emu.o util.o imd.o $(CP_DRIVERS) ../src/sgs/z80dis.o
	$(CC) $(DEBUG) -o cpro hsim.o $(CP_CPU) imd.o z80emu.o util.o $(CP_DRIVERS) ../src/sgs/z80dis.o
	pkill cpro

rmmerge:
	rm -f *-merge 

rmdelta:
	rm -f *-delta 

clean: rmdelta rmmerge
	rm -f *.o maketables hsim dumpfile logfile file.inp

clobber:	clean
	rm -f sim imd imdd hdca-[0-3] hddma-[0-3]
