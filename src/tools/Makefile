SIMTARGETS = \
	ar nm size imd disas dump \
	mnix dcheck icheck ncheck readall
	#mar  hitech

LDFLAGS = -L../lib -lsim
GENERATED = doc.inl zmac.c doc
TARGETS = $(SIMTARGETS) zmac
CXXFLAGS= $(MACHINE)

include ../Makefile.inc

$(SIMTARGETS): ../lib/libsim.a

zmac: zmac.o mio.o doc.o zi80dis.o
	$(CXX) -Wall $(CXXFLAGS) -o zmac zmac.o mio.o doc.o zi80dis.o

doc.o: doc.inl

doc.inl: doc.c doc.txt
	$(CC) -Wall -DMK_DOC -o doc doc.c
	./doc >/dev/null
	rm -f ./doc

zmac.c: zmac.y
	bison --output=zmac.c zmac.y

test: nm disas
	./nm tests/mw.o
	./disas -c 0xa72a 0xa72e 0xa732 0xa736 0xa73a 0xa73e 0xa742 0xa746 \
		0xa48e 0xa4d3 0xa799 0xa79a 0xa4eb 0xa2de \
		./m167 >m167.out

