SOURCES = decomp.c z80.c decomp.h
ALLSOURCES = $(SOURCES) Makefile README
OBJECTS = decomp testvec z80test
OFILES = decomp.o z80.o
CFLAGS = -g -fcolor-diagnostics

decomp: $(OFILES)
	cc -o decomp $(OFILES)

clean:
	rm -f $(OFILES) $(OBJECTS)

z80.o: z80.c decomp.h

decomp.o: decomp.c decomp.h

z80test: z80test.c
	cc -o z80test z80test.c

testvec: z80test
	./z80test >testvec

test: decomp
	./decomp -l 0x122 -l 0x195 -l 0x1e6 -l 0x1ef -l 0x225 -l 0x5a8 -l 0x692 testvec
	# ./decomp -c 0x8982=2 P1.COM

commit:
	git commit
