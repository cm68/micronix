#
# micronix makefile for cc compiler driver.  
# handles both hitech and whitesmith's c
#
all:	cc

cc : cc.c
	cc -o cc cc.c -lS -lS

install: cc /old/cc
	cp cc /bin/cc

/old/cc:
	cp /bin/cc /old/cc

clean: ; rm cc *.o
