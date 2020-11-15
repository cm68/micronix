all:	cp

cp : cp.c
	cc -o cp cp.c -lS

install: cp /old/cp
	./cp cp /bin/cp

/old/cp:
	./cp /bin/cp /old/cp

clean: ; /bin/rm cp *.o
