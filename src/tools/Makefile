#
# build host tools
#
# these are things like native assemblers, disassemblers, file system checkers,
# extractors, inspectors, etc
#
# tools/Makefile
#
# Changed: <2023-06-19 06:09:52 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

SIMTARGETS = \
	ar nm size imd disas dump \
	mnix dcheck icheck ncheck readall imdd dumpsec setdev
	#mar  hitech

LDFLAGS = -lcurses
GENERATED = doc.inl zmac.c doc
TARGETS = $(SIMTARGETS) zmac

include ../Makefile.inc

$(SIMTARGETS): ../lib/libsim.a

nm:	nm.o
	$(CC) $(CFLAGS) -o nm nm.o ../lib/disz80.o ../lib/util.o ../lib/whitesmith.o ../lib/mnix_sys.c $(LDFLAGS)

imd:	imd.o
	$(CC) $(CFLAGS) -o imd imd.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

dumpsec: dumpsec.o
	$(CC) $(CFLAGS) -o dumpsec dumpsec.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

disas:	disas.o
	$(CC) $(CFLAGS) -o disas disas.o ../lib/mnix_sys.o $(LDFLAGS)

mnix:	mnix.o mkfsfunc.o
	$(CC) $(CFLAGS) -o mnix mnix.o mkfsfunc.o ../lib/fslib.o ../lib/fslibfunc.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

# the mkfs worker, shared with cmd/mkfs and compiled here for the host.
mkfsfunc.o: ../micronix/cmd/mkfs/mkfsfunc.c
	$(CC) $(CFLAGS) -I../micronix/include -c ../micronix/cmd/mkfs/mkfsfunc.c -o $@

# the checkers and setdev moved into cmd/; compile them from there.  The
# checkers include <fslib.h> (host) and <types.h>/<sys/...> (micronix),
# so they need both include directories, in that order.
dcheck.o: ../micronix/cmd/dcheck/dcheck.c
	$(CC) $(CFLAGS) -I../include -I../micronix/include -c ../micronix/cmd/dcheck/dcheck.c -o $@

icheck.o: ../micronix/cmd/icheck/icheck.c
	$(CC) $(CFLAGS) -I../include -I../micronix/include -c ../micronix/cmd/icheck/icheck.c -o $@

ncheck.o: ../micronix/cmd/ncheck/ncheck.c
	$(CC) $(CFLAGS) -I../include -I../micronix/include -c ../micronix/cmd/ncheck/ncheck.c -o $@

setdev.o: ../micronix/cmd/setdev/setdev.c
	$(CC) $(CFLAGS) -c ../micronix/cmd/setdev/setdev.c -o $@

dcheck:	dcheck.o
	$(CC) $(CFLAGS) -o dcheck dcheck.o ../lib/fslib.o ../lib/fslibfunc.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

icheck:	icheck.o
	$(CC) $(CFLAGS) -o icheck icheck.o ../lib/fslib.o ../lib/fslibfunc.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

ncheck:	ncheck.o
	$(CC) $(CFLAGS) -o ncheck ncheck.o ../lib/fslib.o ../lib/fslibfunc.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

readall:	readall.o
	$(CC) $(CFLAGS) -o readall readall.o ../lib/fslib.o ../lib/fslibfunc.o ../lib/imdlib.o ../lib/util.o ../lib/lockfile.o $(LDFLAGS)

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

setdev:	setdev.o
	$(CC) $(CFLAGS) -o setdev setdev.o $(LDFLAGS)

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


