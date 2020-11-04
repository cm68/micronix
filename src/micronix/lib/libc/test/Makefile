all: jmptest

jmptest: jmptest.c
	cpp -d unix -i /include/ -x -o jmptest.i jmptest.c 
	cp1 -b0 -m -u -o jmptest.cp1 jmptest.i
	cp2 < jmptest.cp1 >jmptest.s
	as -o jmptest.o jmptest.s
	link -u_main -eb__memory -tb0x100 -dr12 -o jmptest /lib/uhdr.o jmptest.o /lib/libS.a /lib/libmd.a /lib/libu.a /lib/libm.a 

clean:
	rm -f *.i *.cp1 *.o jmptest
