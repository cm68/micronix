CC = gcc
CFLAGS = -g

all: sim

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h
	$(CC) $(CFLAGS) -c $<

micronix.o: micronix.c z80emu.h z80config.h
	$(CC) $(CFLAGS) -c $<

sim: micronix.o z80emu.o ../src/sgs/z80dis.o
	$(CC) micronix.o z80emu.o ../src/sgs/z80dis.o -o $@

test: micronix
	./micronix -t -d ../filesystem ../filesystem/bin/ls

clean:
	rm -f *.o maketables micronix
