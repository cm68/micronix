#
# makefile for stevie (vi clone)
#
# micronix/cmd/vi/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>

OBJECTS = window.o normal.o misccmds.o linefunc.o cmdline.o \
	edit.o main.o

vi: $(OBJECTS)
	cc -o vi $(OBJECTS) -lS -lutil

install: vi
	cp vi /bin/vi

$(OBJECTS) : $*.c stevie.h
	cc -c $*.c

clean:
	rm $(OBJECTS) vi

# vim: tabstop=4 shiftwidth=4 expandtab:
