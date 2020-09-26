DIRS = lib tools usersim

all:
	for i in $(DIRS) ; do \
		(cd $$i ; make ) ; \
	done

clean:
	for i in $(DIRS) ; do \
		(cd $$i ; make clean ) ; \
	done

clobber:
	for i in $(DIRS) ; do \
		(cd $$i ; make clobber ) ; \
	done


