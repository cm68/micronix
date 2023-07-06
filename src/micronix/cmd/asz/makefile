#
# native Makefile for asz assembler
#
# /usr/src/cmd/asz/makefile 
#
# Changed: <2023-07-05 22:46:38 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: asz

asz: main.o asm.o
	cc -v -o asz main.o asm.o -lS

main.o:	main.c
	cc -v -c main.c

asm.o:	asm.c
	cc -v -c asm.c

clean:
	rm *.o

clobber: clean
	rm -f asz

