OBJECTS = make.o makeinit.o makeread.o maketime.o

Make: $(OBJECTS)
	cc -v -o Make make.o makeinit.o makeread.o maketime.o -lS

$(OBJECTS) : $*.c
	cc -c $*.c

make.o: make.c

install: Make
	mv /bin/make /bin/mk
	cp Make /bin/make

clean : ; rm *.o

update :  ; cd ../.. ; make update
