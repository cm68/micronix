#
# Makefile for library tests
#
# lib/test/Makefile
#
# Changed: <2023-06-23 00:29:45 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

test: termtest edittest

termtest:	termtest.c 
	cc -I../../include -o termtest termtest.c ../openx.o

edittest:	edittest.c ../util.o
	cc -I../../include -o edittest edittest.c ../util.o -lncurses
