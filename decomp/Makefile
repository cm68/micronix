SOURCES = decomp.c z80.c decomp.h
ALLSOURCES = $(SOURCES) Makefile README

OFILES = decomp.o z80.o
CFLAGS = -g -fcolor-diagnostics

decomp: $(OFILES)
	cc -o decomp $(OFILES)

clean:
	rm -f $(OFILES)

z80.o: z80.c decomp.h

decomp.o: decomp.c decomp.h

z80test: z80test.c
	cc -o z80test z80test.c

testvec: z80test
	./z80test >testvec

test: decomp
	./decomp testvec
	# ./decomp -c 0x8982=2 P1.COM

commit:
	git commit
