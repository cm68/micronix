all:	msh

	# cc -o msh sh.c -lS
msh : sh.c
	cpp -d unix -i /include/ -x -o sh.i sh.c 
	cp1 -b0 -m -u -o sh.p1 sh.i
	cp2 < sh.p1 >sh.s
	as -o sh.o sh.s
	link -u_main -eb__memory -tb0x100 -r -dr12 -o msh /lib/uhdr.o sh.o /lib/libS.a /lib/libmd.a /lib/libu.a /lib/libm.a 

install: sh /old/sh
	cp msh /bin/msh

/old/sh:
	cp /bin/msh /old/msh

clean: ; /bin/rm msh *.o
