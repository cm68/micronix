#
# this is just a top-level makefile to build the simulator
#

all: src/usersim/sim sim filesystem

sim:
	ln -s src/usersim/sim sim

src/usersim/sim src/tools/readall:
	cd src ; make

test: filesystem  src/usersim/sim
	src/usersim/sim

filesystem: src/tools/readall
	for i in disks/*.image ; do src/tools/readall -d filesystem $$i ; done
	mkdir -p filesystem/usr/src/sys
	cp -r src/kernel/* filesystem/usr/src/sys

clean:
	for dir in src ; do \
		(cd $$dir ; make clean) \
	done

clobber:
	for dir in src ; do \
		(cd $$dir ; make clobber) \
	done
	rm -rf filesystem sim
