#
# makefile for ar
#
# micronix/cmd/ar/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

all:	ar

ar : ar.c
	cc -o ar ar.c -lS

upgrade: ar
	cp ar /bin/ar

install: ar
	cp ar /bin/ar

clean: ; rm ar *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
