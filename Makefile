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
	mkdir -p filesystem/usr/src/sys filesystem/usr/src/cmd
	cp -r src/micronix/* filesystem/usr/src
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
