all:	msh

msh: msh.c
	cc -o msh msh.c -lS

msh.sym : msh.c
	cpp -d unix -i /include/ -x -o msh.i msh.c 
	cp1 -b0 -m -u -o msh.p1 msh.i
	cp2 < msh.p1 > msh.s
	as -o msh.o msh.s
	link -u_main -eb__memory -tb0x100 -r -dr12 -o msh.sym /lib/uhdr.o sh.o /lib/libS.a /lib/libmd.a /lib/libu.a /lib/libm.a 

install: msh
	cp msh /bin/msh
	cp msh.1 /usr/man/man1/msh.1

clean: ; /bin/rm msh *.o
