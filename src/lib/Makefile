#
# simulator library makefile
# contains generic modules useful for any simulator
#
# lib/Makefile
#
# Changed: <2023-07-10 14:45:25 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJECTS = util.o imdlib.o openx.o disz80.o \
	mnix_sys.o whitesmith.o fslib.o gui.o monitor.o hdcdmadisk.o lockfile.o

LIBRARY = libsim.a

TARGETS = $(LIBRARY)

INCLUDE = -I../include

include ../Makefile.inc

$(LIBRARY): $(OBJECTS)
	$(AR) rv $(LIBRARY) $(OBJECTS)
