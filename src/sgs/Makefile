BINS= nm ar
INCPATH=../../include

all: $(BINS)

CFLAGS=-Wno-return-type -I$(INCPATH)

clean:
	rm -f $(BINS) *.o 
	rm -rf *.dSYM

test: nm
	./nm tests/mw.o

nm: nm.c $(INCPATH)/ws.h wslib.o
	gcc $(CFLAGS) -g -o nm nm.c wslib.o

wslib.o: wslib.c $(INCPATH)/ws.h
	gcc $(CFLAGS) -g -c wslib.c

ar: ar.c 
	gcc $(CFLAGS) -g -o ar ar.c
