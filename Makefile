#
# this is just a top-level makefile to build the simulator
#
# Makefile
# Changed: <2023-07-05 23:17:23 curt>
#
# to build this, we have some prerequsites:
# bison, lib32ncurses-dev

all: sim d1 filesystem
	cd src ; make

#
# The simulators, installed as copies into bin/ - not symlinks, which
# break when the tree is moved.  bin/ is also where the cross tools
# (mxccc, mxar, mxnm) land; the simulators are their peers there.
#
sim: bin/sim

d1: bin/d1

bin/sim: src/usersim/sim
	mkdir -p bin
	cp src/usersim/sim bin/sim

bin/d1: src/hwsim/d1/d1
	mkdir -p bin
	cp src/hwsim/d1/d1 bin/d1

src/usersim/sim src/tools/readall:
	cd src ; make

src/hwsim/d1/d1:
	cd src ; make
	cd src/hwsim/d1 ; make

test: filesystem  src/usersim/sim
	src/usersim/sim

DISKS = $(shell cat disks/dist/MICRONIX | grep -v ^# | cut -f 1)
DISKS1 = $(shell cat disks/dist/NEWER | grep -v ^# | cut -f 1)
filesystem: src/tools/readall
	for i in $(DISKS) ; do \
		src/tools/readall -d filesystem disks/dist/$$i ; \
	done
	mkdir -p filesystem/newer
	for i in $(DISKS1) ; do \
		src/tools/readall -d filesystem/newer disks/dist/$$i ; \
	done
	mkdir -p filesystem/usr/src/sys filesystem/usr/src/cmd
	cp src/micronix/*akefile filesystem/usr/src
	for i in cmd include sys lib ; do \
		cp -r src/micronix/$$i filesystem/usr/src ; \
	done
	mkdir -p filesystem/old
	echo "path /bin /usr/bin" > filesystem/.sh

clean:
	for dir in src ; do \
		(cd $$dir ; make clean) \
	done

clobber:
	for dir in src ; do \
		(cd $$dir ; make clobber) \
	done
	rm -rf filesystem bin sim

rebuildfs:
	rm -rf filesystem
	$(MAKE) filesystem
