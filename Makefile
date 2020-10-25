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
	echo "cd /usr/src/sys ; make" > filesystem/rebuild

newkernel: filesystem src/usersim/sim
	for i in src/kernel/* ; do \
		if ! cmp -s $$i filesystem/usr/src/sys/`basename $$i` ; then \
			echo different: $$i ; \
			cp $$i filesystem/usr/src/sys ; \
		fi ; \
	done
	-./sim /bin/sh rebuild
	cp filesystem/usr/src/sys/unix kernels/micronix.new
	cd kernels ; make
	
clean:
	for dir in src ; do \
		(cd $$dir ; make clean) \
	done

clobber:
	for dir in src ; do \
		(cd $$dir ; make clobber) \
	done
	rm -rf filesystem sim
