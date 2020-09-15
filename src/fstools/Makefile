INCPATH=../../include
CHECKS = ncheck icheck dcheck
BINS= $(CHECKS) dumpfs readall mnix

all: $(BINS)

CFLAGS=-Wno-return-type -I$(INCPATH)

clean:
	rm -f *.o 
	rm -rf *.dSYM

clobber: clean
	rm -f $(BINS)
	
refresh:
	rm testfs
	make testfs

testfs:
	cp ../../images/1010-8_stand-alone.image testfs

test: $(CHECKS)
	for i in $(CHECKS) ; do \
		echo $$i ; \
		./$$i testfs ; \
	done

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

readall: fslib.o fs.h readall.c
	gcc $(CFLAGS) -g -o readall readall.c fslib.o

mnix: fslib.o fs.h mnix.c
	gcc $(CFLAGS) -g -o mnix mnix.c fslib.o
