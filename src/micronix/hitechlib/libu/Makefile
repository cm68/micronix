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
