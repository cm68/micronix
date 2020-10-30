all:	ls

ls : ls.c qsort.o
	cc -o ls ls.c qsort.o -lS

qsort.o : qsort.c ; cc -c qsort.c

install: ls
	cp ls /bin/ls

clean: ; rm ls *.o
