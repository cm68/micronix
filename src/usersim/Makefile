#
# Makefile for user mode simulator
#
# usersim/Makefile
#
# Changed: <2023-06-20 12:05:59 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

LDFLAGS = -L../lib -lsim -lcurses -lchips # -lmem

OBJECTS = usersim.o
TARGETS = sim

include ../Makefile.inc

# these sources are K&R-era C.  A gcc new enough to make an implicit
# declaration an error rather than a warning will not build them at
# all, which is why the binary here had outlived its source.
CFLAGS += -std=gnu89

sim: $(OBJECTS) ../lib/libsim.a
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

usersim.o: usersim.c

test: sim
	./sim -d ../../filesystem ../../filesystem/bin/ls
