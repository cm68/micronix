#
# makefile for init
#
# micronix/cmd/init/Makefile
#
# Changed: <2023-07-01 07:02:14 curt>

all: init

init: init.c
	cc -I/usr/include -o init init.c -lS

install: init
	cp init /bin/init

upgrade: init
	cp /bin/init /old/init
	cp init /bin/init

clean:
	rm init *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
