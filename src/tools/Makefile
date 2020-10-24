INCLUDE = -I../include -I../kernel

TARGETS = \
	ar nm size imd disas dump \
	mnix dcheck icheck ncheck readall
	#mar  hitech

LDFLAGS = -L../lib -lsim

include ../Makefile.inc

$(TARGETS): ../lib/libsim.a

test: nm disas
	./nm tests/mw.o
	./disas -c 0xa72a 0xa72e 0xa732 0xa736 0xa73a 0xa73e 0xa742 0xa746 \
		0xa48e 0xa4d3 0xa799 0xa79a 0xa4eb 0xa2de \
		./m167 >m167.out

