#
# %W% - makefile for Z80/Z280 disassembler
#
FLAVOR=-DZ80
#FLAVOR=-DZ280

CC=cc
#CC=gcc

LINT=lint

DEBUG=-g
LDFLAGS=-Bstatic

CFLAGS=$(PROTO) $(FLAVOR) $(DEBUG)

LINTFLAGS=$(FLAVOR) -abchx

all: disas dump

dump:	dump.c
	$(CC) $(CFLAGS) dump.c $(LDFLAGS) -o dump

disas: disas.c
	$(CC) $(CFLAGS) disas.c $(LDFLAGS) -o disas

tags:
	ctags disas.c

disas.o: Makefile

clean:
	rm -f *.o core mon.out gmon.out DDTSTART.ASM DDTEND.ASM DDT.hex

lint:
	$(LINT) $(LINTFLAGS) disas.c

test:	disas dump
	./disas -a 0x100 -e 0x18b DDT.COM >DDTSTART.ASM
	./disas -s 0x100 -a 0 -f DDT.SYM DDT.COM >DDTEND.ASM
	./dump DDT.COM

install:	disas dump
	sudo cp dump disas /usr/local/bin

clobber: clean
	rm -f dump disas
