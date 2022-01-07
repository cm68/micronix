#
# micronix makefile for cc compiler driver.  
# handles both hitech and whitesmith's c
#
all:	cc

cc : cc.c
	cc -o cc cc.c -lS -lS

upgrade: cc
	cp /bin/cc /old/cc
	cp cc /bin/cc

install: cc
	cp cc /bin/cc

clean: ; rm cc *.o
