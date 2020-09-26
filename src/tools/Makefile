INCLUDE = -I../include -I../../include

TARGETS = \
	ar nm size imd disas dump \
	mnix dcheck icheck ncheck readall
	#mar  hitech

LDFLAGS = -L../lib -lsim

include ../Makefile.inc

$(TARGETS): ../lib/libsim.a

test: nm
	./nm tests/mw.o

