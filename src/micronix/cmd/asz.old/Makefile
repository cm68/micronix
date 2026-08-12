#
# native Makefile for asz assembler
#
# /usr/src/cmd/asz/Makefile 
#
# Changed: <2023-07-05 22:46:38 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: asz

asz: main.o asm.o
	cc -I/usr/include -v -o asz main.o asm.o -lS

main.o:	main.c
	cc -I/usr/include -v -c main.c

asm.o:	asm.c
	cc -I/usr/include -v -c asm.c

clean:
	rm *.o

clobber: clean
	rm -f asz

