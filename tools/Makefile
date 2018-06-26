BINS= nm ncheck icheck dcheck dumpfs copyall dis

all: test $(BINS)

CFLAGS=-Wno-return-type

clean:
	rm -f $(BINS) *.o 
	rm -rf *.dSYM

test: testdis

nm: nm.c ws.h wslib.o
	gcc $(CFLAGS) -g -o nm nm.c wslib.o

wslib.o: wslib.c ws.h
	gcc $(CFLAGS) -g -c wslib.c

dumpfs: dumpfs.c fslib.o fs.h
	gcc $(CFLAGS) -g -o dumpfs dumpfs.c fslib.o

fslib.o: fslib.c fs.h
	gcc $(CFLAGS) -g -c fslib.c

dcheck: fslib.o fs.h dcheck.c
	gcc $(CFLAGS) -g -o dcheck dcheck.c fslib.o

ncheck: fslib.o fs.h ncheck.c
	gcc $(CFLAGS) -g -o ncheck ncheck.c fslib.o

icheck: fslib.o fs.h icheck.c
	gcc $(CFLAGS) -g -o icheck icheck.c fslib.o

copyall: fslib.o fs.h copyall.c
	gcc $(CFLAGS) -g -o copyall copyall.c fslib.o

dis: dis.o input.o match.o util.o wslib.o
	gcc $(CFLAGS) -g -o dis dis.o input.o match.o util.o wslib.o

dis.o: dis.c dis.h
	gcc $(CFLAGS) -g -c dis.c

input.o: input.c dis.h
	gcc $(CFLAGS) -g -c input.c

match.o: match.c dis.h
	gcc $(CFLAGS) -g -c match.c

util.o: util.c dis.h
	gcc $(CFLAGS) -g -c util.c

testdis: dis
	./dis test/MON447.HEX
	./dis test/LIBC.REL
	./dis test/mw.o
