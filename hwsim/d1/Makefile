CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -I../common

TARGETS=d1
DRIVERS=djdma.o multio.o hddma.o hdca.o mpz80.o

all: $(TARGETS)

mpz80.o: mpz80.c
	$(CC) $(CFLAGS) -c mpz80.c

multio.o: multio.c
	$(CC) $(CFLAGS) -c multio.c

djdma.o: djdma.c
	$(CC) $(CFLAGS) -c djdma.c

hddma.o: hddma.c
	$(CC) $(CFLAGS) -c hddma.c

hdca.o: hdca.c
	$(CC) $(CFLAGS) -c hdca.c

d1: $(DRIVERS) $(SIM)
	$(CC) $(DEBUG) -o d1 $(DRIVERS) $(SIM) -L../common -lsim -L../z80 -lz80
	-pkill d1

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out

clobber:	clean
	rm -f $(TARGETS)
