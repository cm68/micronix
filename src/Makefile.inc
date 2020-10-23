#
# set this for a 32 bit build.  if you don't specify, it will use the native one
#MACHINE = -m32

CC = gcc 
WARNS = -Wno-builtin-declaration-mismatch -Wformat-overflow=0
GCCFLAGS = $(MACHINE) -fdiagnostics-color=always
DEBUG = -g
CFLAGS = $(DEBUG) $(INCLUDE) $(WARNS) $(GCCFLAGS) $(DEFINES)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

all: $(TARGETS)

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out

clobber:	clean
	rm -f $(TARGETS)
