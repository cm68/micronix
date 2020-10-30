Make: make.o makeinit.o makeread.o maketime.o hexdump.o
	cc -o Make make.o makeinit.o makeread.o maketime.o hexdump.o -lS

make.o: make.c
	cc -c make.c

makeread.o: makeread.c
	cc -c makeread.c

makeinit.o: makeinit.c
	cc -c makeinit.c

maketime.o: maketime.c
	cc -c maketime.c

hexdump.o: hexdump.c
	cc -c hexdump.c

install:
	mv /bin/make /bin/mk
	cp Make /bin/make

clean : ; rm *.o
