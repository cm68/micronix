OBJECTS = qsort.o setjmp.o 

all: $(OBJECTS)

install: all
	lib /lib/libS.a -rv $(OBJECTS)
	
qsort.o: qsort.c
	cc -c qsort.c

setjmp.o: setjmp.s
	as -o setjmp.o setjmp.s

clean:
	rm *.o
