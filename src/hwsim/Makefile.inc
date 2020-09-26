AR = ar
CC = gcc
DEBUG = -g
CCFLAGS = -Wno-builtin-declaration-mismatch -fdiagnostics-color=always
CFLAGS = $(DEBUG) -I../common -Wno-builtin-declaration-mismatch -fdiagnostics-color=always

%.a: %.o
	$(AR) -rv $@ $?

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(TARGETS)

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out

clobber:	clean
	rm -f $(TARGETS)
