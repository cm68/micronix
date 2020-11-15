all:	ls

ls : ls.c
	cc -o ls ls.c -lS

install: ls /old/ls
	cp ls /bin/ls

/old/ls:
	cp /bin/ls /old/ls

clean: ; rm ls *.o
