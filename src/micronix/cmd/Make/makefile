#
# a makefile to make the make that replaces the micronix make
#
OBJECTS = make.o makeinit.o makeread.o maketime.o

Make: $(OBJECTS)
	cc -o Make make.o makeinit.o makeread.o maketime.o -lutil -lS

$(OBJECTS) : $*.c
	cc -c $*.c

make.o: make.c

upgrade: Make
	cp /bin/make /old/make
	cp Make /bin/make
	
install: Make
	cp Make /bin/make

clean : ; rm $(OBJECTS)

update :  ; cd ../.. ; make update
