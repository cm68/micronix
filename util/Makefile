TESTS = termtest
TOOLS = imd
OBJECTS = util.o imdlib.o openx.o disz80.o mnix_sys.o whitesmith.o
LIBRARY = libsim.a
TARGETS = $(LIBRARY) $(TOOLS) $(TESTS)
LDFLAGS = -L. -lsim
INCLUDE = -I../include

include ../Makefile.inc

$(TESTS) $(TOOLS): $(LIBRARY)
$(LIBRARY):	$(OBJECTS)

$(LIBRARY): $(OBJECTS)
	ar rv $(LIBRARY) $(OBJECTS)

util.o: util.c util.h
imdlib.o: imdlib.c

termtest: termtest.c
	$(CC) $(CFLAGS) -o termtest termtest.c $(LDFLAGS)

imd: imd.c
	$(CC) $(CFLAGS) -o imd imd.c $(LDFLAGS)
