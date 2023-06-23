#
# rebuild the micronix libc
#
# micronix/lib/libc/Makefile
#
# Changed: <2023-06-23 01:00:03 curt>

ASOBJS = setjmp.o
COBJS = qsort.o malloc.o atoi.o \
	strdup.o index.o strncmp.o rindex.o \
	opendir.o mkdir.o

OBJECTS = $(ASOBJS) $(COBJS)

all: $(OBJECTS)

install: all
	lib /lib/libS.a -rv $(OBJECTS)

upgrade: all
	cp /lib/libS.a /old
	lib /lib/libS.a -rv $(OBJECTS)

$(COBJS): $*.c
	cc -c $*.c

$(ASOBJS): $*.s
	as -o $*.o $*.s

clean:
	rm $(OBJECTS)

# vim: tabstop=4 shiftwidth=4 expandtab:

