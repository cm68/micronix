all:	mkdir

mkdir : mkdir.c
	cc -o mkdir mkdir.c -lS

install: mkdir
	cp mkdir /bin/mkdir

upgrade:  mkdir
	cp /bin/mkdir /old/mkdir
	cp mkdir /bin/mkdir

clean: ; rm mkdir *.o
