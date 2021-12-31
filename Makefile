#
# this is just a top-level makefile to build the simulator
#
# Makefile
# Changed: <2021-12-30 18:36:25 curt>
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
	mkdir -p filesystem/usr/src/sys filesystem/usr/src/cmd filesystem/hitech
	cp -r src/micronix/* filesystem/usr/src
	for i in $$(find src/hitech/bin -type f -print); do cp $$i filesystem/hitech ; done
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
