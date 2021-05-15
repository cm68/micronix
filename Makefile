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

DISKS = disks/dist/101*.IMD disks/dist/1070*
filesystem: src/tools/readall
	for i in $(DISKS) ; do src/tools/readall -d filesystem $$i ; done
	mkdir -p filesystem/usr/src/sys filesystem/usr/src/cmd
	cp -r src/micronix/* filesystem/usr/src
	cp src/hitech/bin/* filesystem/hitech
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
