#
# makefile for pwd
#
# micronix/cmd/pwd/Makefile
#
# Changed: <2023-07-29 11:22:47 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I/usr/include

all:	pwd

pwd : pwd.c
	cc $(INCLUDE) -o pwd pwd.c

install: pwd
	cp pwd /bin/pwd

upgrade: pwd
	cp /bin/pwd /old/pwd
	cp pwd /bin/pwd

clean: ; rm pwd *.o
