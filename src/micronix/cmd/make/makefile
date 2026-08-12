#
# Makefile for replacement make
# this file is preferred by this make, because it is named as lower case
# 'makefile'
#
# cmd/Make/makefile
#
# Changed: <2023-07-12 13:06:11 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJECTS = make.o makeinit.o makeread.o maketime.o

Make: $(OBJECTS)
	cc -o Make make.o makeinit.o makeread.o maketime.o -lS

$(OBJECTS) : $*.c
	cc -c $*.c

install: Make
	cp Make /bin/make

clean:
	rm $(OBJECTS)

clobber: clean
	rm -f Make
