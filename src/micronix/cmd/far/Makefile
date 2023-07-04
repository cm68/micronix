#
# makefile for far
#
# micronix/cmd/far/Makefile
#
# Changed: <2023-06-23 01:08:21 curt>


all:	far

far : far1.o far2.o far3.o far4.o far5.o 
	cc -o far far1.o far2.o far3.o far4.o far5.o -lS

far1.o : far1.c ; cc -cz far1.c
far2.o : far2.c ; cc -cz far2.c
far3.o : far3.c ; cc -cz far3.c
far4.o : far4.c ; cc -cz far4.c
far5.o : far5.c ; cc -cz far5.c

clean: ; rm *.o far

upgrade: far
	cp /bin/bar /old/far
	cp far /bin/far

install: far
	cp far /bin/far

# vim: tabstop=4 shiftwidth=4 noexpandtab:
