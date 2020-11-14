OBJECTS = hexdump.o logmsg.o
DESTLIB = /lib/libutil.a

all: $(OBJECTS)

install: $(DESTLIB) $(OBJECTS)
	lib $(DESTLIB) -rv $(OBJECTS)

$(OBJECTS): $*.c
	cc -c $*.c

$(DESTLIB):
	cp libempty.a $(DESTLIB)

clean: 
	rm $(OBJECTS)
