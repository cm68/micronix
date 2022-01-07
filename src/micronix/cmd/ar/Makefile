all:	ar

ar : ar.c
	cc -o ar ar.c -lS

upgrade: ar
	cp ar /bin/ar

install: ar
	cp ar /bin/ar

clean: ; rm ar *.o
