all:	ls

ls : ls.c
	cc -o ls ls.c -lS

install:
	cp ls /bin/ls

upgrade: ls
	cp /bin/ls /old/ls
	cp ls /bin/ls

clean: ; rm ls *.o
