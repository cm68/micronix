test: termtest edittest

termtest:	termtest.c 
	cc -I../../include -o termtest termtest.c ../openx.o

edittest:	edittest.c ../util.o
	cc -I../../include -o edittest edittest.c ../util.o -lncurses
