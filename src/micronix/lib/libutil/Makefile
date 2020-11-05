OBJECTS = hexdump.o

all: $(OBJECTS)

install: all
	lib /lib/libutil.a -rv $(OBJECTS)

hexdump.o: hexdump.c
	cc -c hexdump.c

clean: 
	rm $(OBJECTS)
