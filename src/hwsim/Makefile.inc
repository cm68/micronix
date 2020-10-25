AR = ar
CC = gcc
DEBUG = -g
WARNS = -Wno-builtin-declaration-mismatch -fdiagnostics-color=always
INCLUDES = -I../../include -I../common
CFLAGS = $(DEBUG) $(INCLUDES) $(WARNS)

%.a: %.o
	$(AR) -rv $@ $?

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(TARGETS)

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out

clobber:	clean
	rm -f $(TARGETS)
