INCPATH=../../include
BINS= ncheck icheck dcheck dumpfs

all: test $(BINS)

CFLAGS=-Wno-return-type -I$(INCPATH)

clean:
	rm -f $(BINS) *.o 
	rm -rf *.dSYM

test:
	./ncheck tests/1014*

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

