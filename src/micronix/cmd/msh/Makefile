#
# makefile for msh
#
# micronix/cmd/msh/Makefile
#
# Changed: <2023-06-23 01:15:46 curt>

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
	cp *.1 /usr/man/man1/

clean: ; /bin/rm msh *.o

# vim: tabstop=4 shiftwidth=4 expandtab:
