#
# makefile for stevie (vi clone)
#
# micronix/cmd/vi/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

TARGET = vi

OBJECTS = window.o normal.o misccmds.o linefunc.o cmdline.o \
	edit.o main.o

$(TARGET): $(OBJECTS)
	cc -o vi $(OBJECTS) -lS -lutil

install: vi
	cp vi /bin/vi

$(OBJECTS) : $*.c stevie.h
	cc -I/usr/include -c $*.c

clean:
	rm $(OBJECTS) $(TARGET)
