DIRS = z80 common d1

all:
	for i in $(DIRS) ; do (cd $$i ; make -k) ; done

clean:
	for i in $(DIRS) ; do (cd $$i ; make clean) ; done

clobber:	clean
	for i in $(DIRS) ; do (cd $$i ; make clobber) ; done
