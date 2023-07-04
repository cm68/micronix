#
# makefile for cp
#
# micronix/cmd/cp/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

all:	cp

cp : cp.c
	cc -o cp cp.c -lS

upgrade: cp
	./cp /bin/cp /old/cp
	./cp cp /bin/cp

install: cp
	./cp cp /bin/cp

clean: ; /bin/rm cp *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
