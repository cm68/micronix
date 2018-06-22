BINS= nm ncheck icheck dcheck dumpfs copyall

all: $(BINS)

CFLAGS=-Wno-return-type

clean:
	rm -f $(BINS) *.o 
	rm -rf *.dSYM

nm: nm.c ws.h
	gcc $(CFLAGS) -g -o nm nm.c

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
