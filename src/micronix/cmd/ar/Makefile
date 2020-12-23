all:	ar

ar : ar.c
	cc -o ar ar.c -lS

install: ar /oar/ar
	cp ar /bin/ar

/oar/ar:
	cp /bin/ar /oar/ar

clean: ; rm ar *.o
