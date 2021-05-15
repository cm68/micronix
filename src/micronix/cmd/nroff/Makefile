OBJS = \
	command.o dev.o escape.o io.o low.o \
	macros.o main.o strings.o text.o

all: nroff

nroff: $(OBJS)
	cc -o nroff $(OBJS) -lS

$(OBJS): $*.c
	cc -c $*.c

