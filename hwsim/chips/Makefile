CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -I../common

OBJECTS= shim.o
LIB=libz80.a

all: $(LIB)

$(LIB):	$(OBJECTS)
	ar -rv $(LIB) $(OBJECTS)

shim.o: shim.c z80.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIB)
