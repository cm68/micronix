CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -I../common

OBJECTS= simglb.o sim1.o sim2.o sim3.o sim4.o sim5.o sim6.o sim7.o shim.o
LIB=libz80.a

all: $(LIB)

$(LIB):	$(OBJECTS)
	ar -rv $(LIB) $(OBJECTS)

shim.o: shim.c
	$(CC) $(CFLAGS) -c $<

simglb.o: simglb.c simglb.h
	$(CC) $(CFLAGS) -c $<

sim1.o: sim1.c
	$(CC) $(CFLAGS) -c $<

sim2.o: sim2.c
	$(CC) $(CFLAGS) -c $<

sim3.o: sim3.c
	$(CC) $(CFLAGS) -c $<

sim4.o: sim4.c
	$(CC) $(CFLAGS) -c $<

sim5.o: sim5.c
	$(CC) $(CFLAGS) -c $<

sim6.o: sim6.c
	$(CC) $(CFLAGS) -c $<

sim7.o: sim7.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIB)
