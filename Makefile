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
# The simulators, installed as symlinks into bin/.  bin/ is also where
# the cross tools (mxccc, mxar, mxnm) land; the simulators are their
# peers there.  The link is relative so that moving the whole tree
# keeps it pointing at its target, and -f replaces a plain file left
# behind by the copy-style rule this used to be.
#
sim: bin/sim

d1: bin/d1

bin/sim: src/usersim/sim
	mkdir -p bin
	ln -sf ../src/usersim/sim bin/sim

bin/d1: src/hwsim/d1/d1
	mkdir -p bin
	ln -sf ../src/hwsim/d1/d1 bin/d1

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

#
# The filesystem skeleton, from source alone: the directory tree, /dev
# (from src/micronix/dev/devlist, the same list create_vol mknod's onto a
# volume), and the shell's startup files.  This is the part of `filesystem`
# that does not need the distribution disks - see docs/DISTRIBUTION-FREE.md.
# It is a separate target because `filesystem` still reads the distribution
# binaries; when the porting gap in that doc is closed, this folds in and
# the readall loops go away.
#
DEVROOT = filesystem/dev
.PHONY: filesystem-skel
filesystem-skel:
	mkdir -p filesystem/bin filesystem/dev filesystem/etc filesystem/lib \
		filesystem/libexec filesystem/include filesystem/tmp filesystem/old \
		filesystem/usr/bin filesystem/usr/lib filesystem/usr/man \
		filesystem/usr/include filesystem/usr/src filesystem/usr/adm \
		filesystem/usr/spool
	rm -rf $(DEVROOT) && mkdir $(DEVROOT)
	grep -v '^#' src/micronix/dev/devlist | while read t m n name; do \
		[ -z "$$name" ] && continue; \
		ln -sf "$${t}dev($$m,$$n)" $(DEVROOT)/$$name; \
	done
	echo "path /bin /usr/bin" > filesystem/.sh
	: > filesystem/.login

clean:
	for dir in src src/hwsim src/micronix ; do \
		(cd $$dir ; make clean) \
	done

clobber:
	for dir in src src/hwsim src/micronix ; do \
		(cd $$dir ; make clobber) \
	done
	rm -rf filesystem bin libexec ccc lib usr sim

rebuildfs:
	rm -rf filesystem
	$(MAKE) filesystem

#
# The bootable install diskettes: one per medium stand/boot builds a
# boot image for, made by the script in disks/bootdisks.  Three things
# have to exist first and each is a different part of the tree - the
# boot images (stand, cross built), the installer (src/tools) and the
# userland they carry (make filesystem).  The medium is named once, to
# the script; the geometry, the filesystem size and the diskette's own
# root minor all come out of the tree from there.
#
# They land in disks/bootdisks as <medium>.img and are gitignored build
# output, like a .vol - generated, never committed.
#
BOOTDISKS = d8ss d8ds d5ds

bootdisks: filesystem src/tools/mnix src/tools/setdev
	$(MAKE) -C src/micronix/stand/boot djbootimgs bootimgs
	for i in $(BOOTDISKS) ; do \
		disks/bootdisks/create_bootdisk $$i || exit 1 ; \
	done
