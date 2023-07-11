#
# Makefile for user mode simulator
#
# usersim/Makefile
#
# Changed: <2023-06-20 12:05:59 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

LDFLAGS = -L../lib -lsim -lcurses -lchips -lmem

OBJECTS = usersim.o
TARGETS = sim

include ../Makefile.inc

sim: $(OBJECTS) ../lib/libsim.a
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

usersim.o: usersim.c

test: sim
	./sim -d ../../filesystem ../../filesystem/bin/ls
