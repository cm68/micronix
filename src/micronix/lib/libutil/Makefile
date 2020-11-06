OBJECTS = hexdump.o logmsg.o

all: $(OBJECTS)

install: all
	lib /lib/libutil.a -rv $(OBJECTS)

$(OBJECTS): $*.c
	cc -c $*.c

clean: 
	rm $(OBJECTS)
