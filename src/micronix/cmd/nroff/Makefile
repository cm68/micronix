#
# makefile for nroff
#
# micronix/cmd/nroff/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

OBJS = \
	command.o dev.o escape.o io.o low.o \
	macros.o main.o strings.o text.o

all: nroff

nroff: $(OBJS)
	cc -o nroff $(OBJS) -lS

$(OBJS): $*.c
	cc -c $*.c

# vim: tabstop=4 shiftwidth=4 expandtab:
