#
# makefile for form
#
# micronix/cmd/form/Makefile
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	form

form : form.c
	cc -I/usr/include -o form form.c -lS

# a host build, for comparing output against the original binary
# running under the simulator
host : form.c
	gcc -std=gnu89 -w -o form.host form.c

install: form
	cp form /bin/form

clean: ; /bin/rm -f form form.host *.o
