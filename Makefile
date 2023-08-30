#
# this is just a top-level makefile to build the simulator
#
# Makefile
# Changed: <2023-07-05 23:17:23 curt>
#
# to build this, we have some prerequsites:
# bison, lib32ncurses-dev

all: sim filesystem
	cd src ; make

sim:
	ln -s src/usersim/sim sim

src/usersim/sim src/tools/readall:
	cd src ; make

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
	rm -rf filesystem sim

rebuildfs:
	rm -rf filesystem
	$(MAKE) filesystem
