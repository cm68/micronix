   #OBJECTS = window.o normal.o misccmds.o linefunc.o hexchars.o cmdline.o \
	edit.o main.o

vi: $(OBJECTS)
	cc -o vi $(OBJECTS) -lS -lutil

install: vi
	cp vi /bin/vi

$(OBJECTS) : $*.c stevie.h
	cc -c $*.c

clean:
	rm $(OBJECTS) vi
