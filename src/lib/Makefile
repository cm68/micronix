OBJECTS = util.o imdlib.o openx.o disz80.o \
	mnix_sys.o whitesmith.o fslib.o
LIBRARY = libsim.a
TARGETS = $(LIBRARY) 
INCLUDE = -I../include -I../micronix/sys

include ../Makefile.inc

$(LIBRARY): $(OBJECTS)
	ar rv $(LIBRARY) $(OBJECTS)

