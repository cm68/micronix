#
# makefile for whitesmith toolchain
#
# micronix/cmd/whitesmith/lib
#
# Changed: <2023-07-11 05:55:46 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:
	cd libws ; make
	cd libm ; make
	cd libu ; make
	
install: 

clean: 
	cd libws ; make clean
	cd libm ; make clean
	cd libu ; make clean

clobber: clean
	cd libws ; make clobber
	cd libm ; make clobber
	cd libu ; make clobber
