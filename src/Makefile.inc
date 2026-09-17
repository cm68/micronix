#
# this makefile fragment is included by low level makefiles to put
# as much host information as possible in one place
#
# src/Makefile.inc
#
# Changed: <2021-12-23 15:27:20 curt>

# set this for a 32 bit build.  if you don't specify, it will use the native one
MACHINE = -m32

AR = ar
CC = gcc 
WARNS = -Wno-builtin-declaration-mismatch -Wformat-overflow=0 -Wno-pointer-sign \
	-fdiagnostics-color=always

# 64-bit off_t: the trace logfile and the disk images routinely exceed
# 2G, and a 32-bit off_t stops every stdio write dead at 0x7fffffff.
GCCFLAGS = $(MACHINE) -D_FILE_OFFSET_BITS=64
# "make NODEBUG=1" builds a hardware-facing simulator: -DNODEBUG compiles
# out the tracing/logging/watchpoints (see util.h/hwsim.h) and drops the
# debug symbols.  The library changes shape under that flag - the trace
# functions disappear - so the two builds use different .a names and can
# coexist side by side.
ifdef NODEBUG
DEBUG =
DEFINES = -DNODEBUG
LIBSUFFIX = -nodbg
OBJSUFFIX = -nodbg
else
DEBUG = -g
DEFINES =
LIBSUFFIX =
OBJSUFFIX =
endif
# the simulator spends all its time in one interpreter loop, and built
# without this it runs at about half speed.  -g stays: we still want the
# symbols for the monitor and for profiling.
OPTIMIZE = -O2
CFLAGS = $(DEBUG) $(OPTIMIZE) $(INCLUDE) $(WARNS) $(GCCFLAGS) $(DEFINES)
CXXFLAGS = $(CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# The -DNODEBUG build keeps its objects separate from the normal ones
# (djdma-nodbg.o beside djdma.o), so the two simulators - d1 and the
# performance d1p - can both be linked and kept around without one
# relinking the other from the wrong objects.
%-nodbg.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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
