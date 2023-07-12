#
# Makefile for better make on micronix
# this make, because it is named with upper case 'Makefile',
# is preferred by the old micronix make
#
# cmd/Make/Makefile
#
# Changed: <2023-07-12 13:02:27 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

install: Make
	cp Make /bin/make

Make: make.o makeread.o makeinit.o maketime.o
	cc -o Make *.o -lS

make.o: make.c
	cc -c make.c

makeread.o: makeread.c
	cc -c makeread.c

makeinit.o: makeinit.c
	cc -c makeinit.c

maketime.o: maketime.c
	cc -c maketime.c

clean:
	rm *.o

clobber: clean
	rm Make
