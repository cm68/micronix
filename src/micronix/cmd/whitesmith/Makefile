#
# makefile for whitesmith toolchain
#
# micronix/cmd/whitesmith
#
# Changed: <2023-07-10 20:16:50 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd lib ; make
	cd asm ; make
	cd pp ; make
	cd p1 ; make
	cd p2 ; make
	cd util ; make
	
install: 

clean: 
	cd lib ; make clean
	cd asm ; make clean
	cd pp ; make clean
	cd p1 ; make clean
	cd p2 ; make clean
	cd util ; make clean

clobber: clean
	cd lib ; make clobber
	cd asm ; make clobber
	cd pp ; make clobber
	cd p1 ; make clobber
	cd p2 ; make clobber
	cd util ; make clobber
