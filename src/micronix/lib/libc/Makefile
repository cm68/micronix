ASOBJS = setjmp.o
COBJS = qsort.o malloc.o strdup.o index.o strncmp.o atoi.o

OBJECTS = $(ASOBJS) $(COBJS)

all: $(OBJECTS)

install: all
	lib /lib/libS.a -rv $(OBJECTS)
	
$(COBJS): $*.c
	cc -c $*.c

$(ASOBJS): $*.s
	as -o $*.o $*.s

clean:
	rm $(OBJECTS)
