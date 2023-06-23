#
# cross compile hitech c library
#
# micronix/hitechlib/libu/Makefile
#
# Changed: <2023-06-23 01:06:16 curt>

LIBRARY = libu.lib
OBJECTS = read.obj write.obj open.obj close.obj fork.obj _exit.obj
CRT = crt0.obj

all: $(LIBRARY) $(CRT)

$(LIBRARY): $(OBJECTS)
	for i in $(OBJECTS) ; do \
		zxcc libr -r $(LIBRARY) -$$i ; \
	done

%.obj: %.c
	zxc -c $<

%.obj: %.as
	zxc -c $<

clean: clobber

clobber:
	rm -f $(OBJECTS) $(LIBRARY) $(CRT)

# vim: tabstop=4 shiftwidth=4 expandtab:

