CC = gcc 
GCCFLAGS = -m32 -Wno-builtin-declaration-mismatch -fdiagnostics-color=always
DEBUG = -g
CFLAGS = $(DEBUG) $(INCLUDE) $(GCCFLAGS) $(DEFINES)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

all: $(TARGETS)

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out

clobber:	clean
	rm -f $(TARGETS)