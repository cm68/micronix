OFILES = decomp.o z80.o
CFLAGS = -g

decomp: $(OFILES)
	cc -o decomp $(OFILES)

clean:
	rm -f $(OFILES)

z80.o: z80.c

decomp.o: decomp.c z80test.h

test: decomp
	 ./decomp -c 0x8982=2 P1.COM
