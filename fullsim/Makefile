CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG)

CP_DRIVERS=ss1.o disk1.o
D1_DRIVERS=djdma.o multio.o hddma.o hdca.o

D1_CPU=mpz80.o
CP_CPU=cpuz.o

all: z80test d1 imd imdd

z80test: z80.o z80test.o
	$(CC) $(CFLAGS) -o z80test z80.o z80test.o

z80test.o: z80test.c z80.h
	$(CC) $(CFLAGS) -c z80test.c

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80.o: z80.c z80.h
	$(CC) $(CFLAGS) -c z80.c

util.o:	util.c sim.h
	$(CC) $(CFLAGS) -c util.c

mpz80.o: mpz80.c
	$(CC) $(CFLAGS) -c mpz80.c

hsim.o: hsim.c z80.h sim.h
	$(CC) $(CFLAGS) -c hsim.c

multio.o: multio.c sim.h
	$(CC) $(CFLAGS) -c multio.c

djdma.o: djdma.c sim.h
	$(CC) $(CFLAGS) -c djdma.c

hddma.o: hddma.c sim.h
	$(CC) $(CFLAGS) -c hddma.c

hdca.o: hdca.c sim.h
	$(CC) $(CFLAGS) -c hdca.c

ss1.o: ss1.c sim.h
	$(CC) $(CFLAGS) -c ss1.c

disk1.o: disk1.c sim.h
	$(CC) $(CFLAGS) -c disk1.c

cpuz.o: cpuz.c sim.h
	$(CC) $(CFLAGS) -c cpuz.c

imd: imd.c sim.h util.o
	$(CC) -DSTAND $(CFLAGS) -o imd imd.c util.o

imdd: imdd.c
	$(CC) -DSTAND $(CFLAGS) -o imdd imdd.c

d1: hsim.o $(D1_CPU) z80.o util.o imd.o $(D1_DRIVERS) ../src/sgs/z80dis.o
	$(CC) $(DEBUG) -o d1 hsim.o $(D1_CPU) imd.o z80.o util.o $(D1_DRIVERS) ../src/sgs/z80dis.o
	-pkill d1

cpro: hsim.o $(CP_CPU) z80.o util.o imd.o $(CP_DRIVERS) ../src/sgs/z80dis.o
	$(CC) $(DEBUG) -o cpro hsim.o $(CP_CPU) imd.o z80.o util.o $(CP_DRIVERS) ../src/sgs/z80dis.o
	pkill cpro

rmmerge:
	rm -f *-merge 

rmdelta:
	rm -f *-delta 

clean: rmdelta rmmerge
	rm -f *.o maketables hsim dumpfile logfile file.inp

clobber:	clean
	rm -f sim imd imdd hdca-[0-3] hddma-[0-3]
