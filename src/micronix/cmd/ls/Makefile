all:	ls

ls : ls.c
	cc -o ls ls.c ../../lib/libc/qsort.o -lS

install: ls
	cp ls /bin/ls

clean: ; rm ls *.o
