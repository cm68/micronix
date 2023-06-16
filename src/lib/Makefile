# simulator library makefile
#
# lib/Makefile
#
# Changed: <2021-12-23 15:38:59 curt>

OBJECTS = util.o imdlib.o openx.o disz80.o \
	mnix_sys.o whitesmith.o fslib.o gui.o
LIBRARY = libsim.a
TARGETS = $(LIBRARY) 
INCLUDE = -I../include

include ../Makefile.inc

$(LIBRARY): $(OBJECTS)
	ar rv $(LIBRARY) $(OBJECTS)

