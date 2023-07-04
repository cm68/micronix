#
# build host tools
#
# these are things like native assemblers, disassemblers, file system checkers,
# extractors, inspectors, etc
#
# tools/Makefile
#
# Changed: <2023-06-19 06:09:52 curt>

SIMTARGETS = \
	ar nm size imd disas dump \
	mnix dcheck icheck ncheck readall imdd
	#mar  hitech

LDFLAGS = -lcurses
GENERATED = doc.inl zmac.c doc
TARGETS = $(SIMTARGETS) zmac
CXXFLAGS= $(MACHINE)

include ../Makefile.inc

$(SIMTARGETS): ../lib/libsim.a

nm:	nm.o
	$(CC) -o nm nm.o ../lib/disz80.o ../lib/util.o ../lib/whitesmith.o ../lib/mnix_sys.c $(LDFLAGS)

imd:	imd.o
	$(CC) -o imd imd.o ../lib/imdlib.o ../lib/util.o $(LDFLAGS)

disas:	disas.o
	$(CC) -o disas disas.o ../lib/mnix_sys.o $(LDFLAGS)

mnix:	mnix.o
	$(CC) -o mnix mnix.o ../lib/fslib.o ../lib/imdlib.o ../lib/util.o $(LDFLAGS)

dcheck:	dcheck.o
	$(CC) -o dcheck dcheck.o ../lib/fslib.o ../lib/imdlib.o ../lib/util.o $(LDFLAGS)

icheck:	icheck.o
	$(CC) -o icheck icheck.o ../lib/fslib.o ../lib/imdlib.o ../lib/util.o $(LDFLAGS)

ncheck:	ncheck.o
	$(CC) -o ncheck ncheck.o ../lib/fslib.o ../lib/imdlib.o ../lib/util.o $(LDFLAGS)

readall:	readall.o
	$(CC) -o readall readall.o ../lib/fslib.o ../lib/imdlib.o ../lib/util.o $(LDFLAGS)

zmac: zmac.o mio.o doc.o zi80dis.o
	$(CXX) -Wall $(CXXFLAGS) -o zmac zmac.o mio.o doc.o zi80dis.o

doc.o: doc.inl

doc.inl: doc.c doc.txt
	$(CC) -Wall -DMK_DOC -o doc doc.c
	./doc >/dev/null
	rm -f ./doc

zmac.c: zmac.y
	bison --output=zmac.c zmac.y

imdd: imdd.c
	cc -o imdd imdd.c

test: disas
	./disas tests/mw.o

test1: disas
	./disas -c 0xa72a 0xa72e 0xa732 0xa736 0xa73a 0xa73e 0xa742 0xa746 \
		0xa48e 0xa4d3 0xa799 0xa79a 0xa4eb 0xa2de \
		./tests/m167 >tests/m167.out

test2: disas
	./disas ../../lib/libm/idiv.o

testd:	dcheck
	for i in ../../disks/dist/*.IMD ; do \
		echo $$i ; \
		./dcheck $$i ; \
	done;

testi:	icheck
	for i in ../../disks/dist/*.IMD ; do \
		echo $$i ; \
		./icheck $$i ; \
	done;

testn:	ncheck
	for i in ../../disks/dist/*.IMD ; do \
		echo $$i ; \
		./ncheck $$i ; \
	done;

install: disas
	cp disas ~/bin

# vim: tabstop=4 shiftwidth=4 expandtab:

