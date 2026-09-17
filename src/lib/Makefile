#
# simulator library makefile
# contains generic modules useful for any simulator
#
# lib/Makefile
#
# Changed: <2023-07-10 14:45:25 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJECTS = util$(OBJSUFFIX).o imdlib$(OBJSUFFIX).o openx$(OBJSUFFIX).o \
	disz80$(OBJSUFFIX).o mnix_sys$(OBJSUFFIX).o whitesmith$(OBJSUFFIX).o \
	fslib$(OBJSUFFIX).o fslibfunc$(OBJSUFFIX).o gui$(OBJSUFFIX).o \
	monitor$(OBJSUFFIX).o hdcdmadisk$(OBJSUFFIX).o lockfile$(OBJSUFFIX).o

LIBRARY = libsim$(LIBSUFFIX).a

TARGETS = $(LIBRARY)

INCLUDE = -I../include

include ../Makefile.inc

# the portable half of the fs library, shared with the micronix build.
# It includes <types.h> and <sys/fs.h>, so it needs the micronix include
# directory as well as this tree's.
fslibfunc$(OBJSUFFIX).o: ../micronix/lib/fslib/fslibfunc.c
	$(CC) $(CFLAGS) -I../micronix/include -c ../micronix/lib/fslib/fslibfunc.c -o $@

$(LIBRARY): $(OBJECTS)
	$(AR) rv $(LIBRARY) $(OBJECTS)
