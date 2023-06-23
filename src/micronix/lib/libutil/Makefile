#
# micronix native additional utility library
#
# micronix/lib/Makefile
#
# Changed: <2021-12-23 15:51:54 curt>

OBJECTS = hexdump.o logmsg.o
DESTLIB = /lib/libutil.a

all: $(OBJECTS)

upgrade:

install: $(DESTLIB) $(OBJECTS)
	lib $(DESTLIB) -rv $(OBJECTS)

$(OBJECTS): $*.c
	cc -c $*.c

$(DESTLIB):
	cp libempty.a $(DESTLIB)

clean: 
	rm $(OBJECTS)

# vim: tabstop=4 shiftwidth=4 expandtab:

