all:	far

far : far1.o far2.o far3.o far4.o far5.o 
	cc -o far far1.o far2.o far3.o far4.o far5.o -lS

far1.o : far1.c ; cc -cz far1.c
far2.o : far2.c ; cc -cz far2.c
far3.o : far3.c ; cc -cz far3.c
far4.o : far4.c ; cc -cz far4.c
far5.o : far5.c ; cc -cz far5.c

clean: ; rm *.o far

install: far /old/far
	cp far /bin/far

/old/far:
	cp /bin/far /old/far
