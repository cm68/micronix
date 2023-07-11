#
# micronix native additional utility library
#
# micronix/lib/Makefile
#
# Changed: <2023-07-10 14:50:02 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJECTS = hexdump.o logmsg.o
DESTLIB = /lib/libutil.a

all: $(OBJECTS)

upgrade:

install: $(DESTLIB) $(OBJECTS)
	lib $(DESTLIB) -rv $(OBJECTS)

$(OBJECTS): $*.c
	cc -i /usr/include/ -c $*.c

$(DESTLIB):
	cp libempty.a $(DESTLIB)

clean: 
	rm $(OBJECTS)
