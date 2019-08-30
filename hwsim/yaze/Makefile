CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -I../common

OBJECTS= yaze.o shim.o
LIB=libyaze.a

all: $(LIB)

$(LIB):	$(OBJECTS)
	ar -rv $(LIB) $(OBJECTS)

shim.o: shim.c
	$(CC) $(CFLAGS) -c $<

yaze.o: yaze.c yaze.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIB)
