#
# this makefile fragment is included by low level makefiles to put
# as much host information as possible in one place
#
# src/Makefile.inc
#
# Changed: <2021-12-23 15:27:20 curt>

# set this for a 32 bit build.  if you don't specify, it will use the native one
#MACHINE = -m32

AR = ar
CC = gcc 
WARNS = -Wno-builtin-declaration-mismatch -Wformat-overflow=0 -Wno-pointer-sign \
	-fdiagnostics-color=always

GCCFLAGS = $(MACHINE)
DEBUG = -g
CFLAGS = $(DEBUG) $(INCLUDE) $(WARNS) $(GCCFLAGS) $(DEFINES)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

all: $(TARGETS)
	for i in $(DIRS) ; do (cd $$i ; make -k) ; done

clean:
	rm -f *.o dumpfile logfile file.inp file.out make.out
	for i in $(DIRS) ; do (cd $$i ; make clean) ; done

clobber:	clean
	rm -f $(TARGETS) $(GENERATED)
	for i in $(DIRS) ; do (cd $$i ; make clobber) ; done
