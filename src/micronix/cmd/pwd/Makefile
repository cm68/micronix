#
# makefile for pwd
#
# micronix/cmd/pwd/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

all:	pwd

pwd : pwd.c
	cc -o pwd pwd.c

install: pwd
	cp pwd /bin/pwd

upgrade: pwd
	cp /bin/pwd /old/pwd
	cp pwd /bin/pwd

clean: ; rm pwd *.o

# vim: tabstop=4 shiftwidth=4 expandtab:
