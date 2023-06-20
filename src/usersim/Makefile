# Makefile for user mode simulator
#
# usersim/Makefile
# Changed: <2023-06-20 12:05:59 curt>
#

LDFLAGS = -L../lib -lsim -lcurses -lchips -lmem

OBJECTS = micronix.o
TARGETS = sim

include ../Makefile.inc

sim: $(OBJECTS) ../lib/libsim.a
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

micronix.o: micronix.c

test: sim
	./sim -d ../../filesystem ../../filesystem/bin/ls

