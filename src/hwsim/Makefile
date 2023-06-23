#
# hardware simulator top level makefile
#
# hwsim/Makefile 
#
# Changed: <2023-06-23 00:27:25 curt>
#
TARGETS = hwsim.o
INCLUDE = -I../include

# specific machine directories
DIRS = d1

include ../Makefile.inc

all: $(TARGETS)
	for i in $(DIRS) ; do (cd $$i ; make -k) ; done

clean:
	for i in $(DIRS) ; do (cd $$i ; make clean) ; done

clobber:	clean
	for i in $(DIRS) ; do (cd $$i ; make clobber) ; done

# vim: tabstop=4 shiftwidth=4 expandtab:

