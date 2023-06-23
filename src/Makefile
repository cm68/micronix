#
# build the simulators
#
# src/Makefile
#
# Changed: <2023-06-23 01:19:37 curt>

DIRS = lib tools usersim # hwsim

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


