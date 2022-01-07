#
# build the simulators
#
# src/Makefile
# Changed: <2021-12-23 15:26:57 curt>
#
DIRS = lib tools hwsim usersim

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


