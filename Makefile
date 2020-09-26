#
# this is just a top-level makefile to build the simulator
#
all:
	cd src ; make

test: src/usersim/sim
	cd src/usersim ; ./sim

unpack:
	cd src/tools; make
	for i in disks/*.image ; do src/tools/readall -d filesystem $$i ; done
	cp -r src/kernel filesystem/usr/src/sys

clean:
	for dir in usersim tools src/sgs src/fstools ; do \
		(cd $$dir ; make clean) \
	done

clobber:
	for dir in usersim tools src/sgs src/fstools ; do \
		(cd $$dir ; make clobber) \
	done
