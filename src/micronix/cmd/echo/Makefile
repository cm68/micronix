#
# makefile for echo
#
# micronix/cmd/echo/Makefile
#
# Changed: <2023-07-10 15:35:25 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	echo

echo : echo.c
	cc -I/usr/include -o echo echo.c -lS

install: echo
	cp echo /bin/echo

clean: ; /bin/rm echo *.o
