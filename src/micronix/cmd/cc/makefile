#
# micronix makefile for cc compiler driver.  
# handles both hitech and whitesmith's c
#
# micronix/cmd/cc/makefile
#
# Changed: <2023-07-10 15:11:18 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	cc

cc : cc.c
	cc -i /usr/include/ -o cc cc.c -lS -lS

upgrade: cc
	cp /bin/cc /old/cc
	cp cc /bin/cc

install: cc
	cp cc /bin/cc

clean: ; rm cc *.o
