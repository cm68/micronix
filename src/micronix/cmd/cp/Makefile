all:	cp

cp : cp.c
	cc -o cp cp.c -lS

upgrade: cp
	./cp /bin/cp /old/cp
	./cp cp /bin/cp

install: cp
	./cp cp /bin/cp

clean: ; /bin/rm cp *.o
