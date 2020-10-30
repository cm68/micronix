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
	cp -r src/kernel/* filesystem/usr/src/sys
	cp -r src/cmd/* filesystem/usr/src/cmd
	echo "cd /usr/src/$1 ; make ; make ; make " > filesystem/rebuild

cmds: filesystem src/usersim/sim
	find src/cmd -type f \! -path \*unused\* | while read i ; do \
		if [ ! -d $$(dirname filesystem/usr/$$i) ] ; then \
			mkdir -p $$(dirname filesystem/usr/$$i) ; \
		fi ; \
		if [ ! -f filesystem/usr/$$i ] ; then \
			cp $$i filesystem/usr/$$i ; \
		fi ; \
		if ! cmp -s $$i filesystem/usr/$$i ; then \
			echo different: $$i ; \
			cp $$i filesystem/usr/$$i ; \
		fi ; \
	done
#	-./sim /bin/sh rebuild cmd
	
newkernel: filesystem src/usersim/sim
	for i in src/kernel/* ; do \
		if [ -f $$i ] && ! cmp -s $$i filesystem/usr/src/sys/`basename $$i` ; then \
			echo different: $$i ; \
			cp $$i filesystem/usr/src/sys ; \
		fi ; \
	done
	rm -f filesystem/usr/src/sys/high.o filesystem/usr/src/sys/main.o filesystem/usr/src/sys/unix
	-./sim /bin/sh rebuild sys
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
