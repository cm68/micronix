CC = gcc
CFLAGS = -Wall -ansi -pedantic -O2 -fomit-frame-pointer

all: micronix

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h
	$(CC) $(CFLAGS) -c $<

micronix.o: micronix.c z80emu.h z80config.h
	$(CC) -Wall -c $<

micronix: micronix.o z80emu.o
	$(CC) micronix.o z80emu.o -o $@

clean:
	rm -f *.o maketables micronix
