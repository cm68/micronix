all:	ls

ls : ls.c
	cc -o ls ls.c -lS

install: ls
	cp ls /bin/ls

clean: ; rm ls *.o
