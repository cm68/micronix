#
# this is just a top-level makefile to build the simulator
#
all:
	cd usersim ; make

clean:
	for dir in usersim tools src/sgs src/fstools ; do \
		(cd $$dir ; make clean) \
	done
clobber:
	for dir in usersim tools src/sgs src/fstools ; do \
		(cd $$dir ; make clobber) \
	done
