INCLUDE = -I../include -I../../include

TARGETS = \
	ar nm size imd disas dump hitech \
	mnix dcheck icheck ncheck readall
	#mar 

LDFLAGS = -L../lib -lsim

include ../Makefile.inc

test: nm
	./nm tests/mw.o
