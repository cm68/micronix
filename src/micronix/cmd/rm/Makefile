all:	rm

rm : rm.c
	cc -o rm rm.c -lS

install: rm /old/rm
	cp rm /bin/rm

/old/rm:
	cp /bin/rm /old/rm

clean: ; /bin/rm rm *.o

test:
	mkdir foo
	mkdir foo/bar
	mkdir foo/bar/baz
	cp * foo/bar/baz
