#
# this is the top-level makefile: the simulators, the image tools, and
# the cross toolchain every GNUmakefile under src/micronix builds with
#
# GNUmakefile
# Changed: <2023-07-05 23:17:23 curt>
#
# to build this, we have some prerequsites:
# bison, lib32ncurses-dev

all: sim d1 filesystem toolchain man
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

#
# The cross toolchain: the compiler and its passes, the host tools in
# bin/, and the runtime archives built with that compiler and put where
# it looks for them (see "toolchain" in src/micronix/GNUmakefile).
# This is what every GNUmakefile under src/micronix needs before it can
# run, so it is part of all; building one directory by hand, "make -C
# src/micronix/cmd/make", then works without a bootstrap.
#
.PHONY: toolchain
toolchain:
	$(MAKE) -C src/micronix toolchain

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

#
# The host manual tree, man/.  Every page in the tree is mirrored there
# by install-man-host, and the two mx tools that read it - mxman and
# mxform - are built into bin/ beside the other mx binaries.  This is
# the host side of what "make install" does for the system below: the
# same MANSRCS, put where mxman can read them without booting anything.
#
# MXROOT anchors the mirror at this checkout and SUDO= leaves it
# writable.  The default, /usr/local/micronix, is for an installed
# tree and is not ours - the same reason bin/ receives the cross tools
# rather than /usr/local/bin.
#
.PHONY: man
man:
	$(MAKE) -C src/micronix/cmd/man host
	$(MAKE) -C src/micronix/cmd/form host
	$(MAKE) -C src/micronix install-man-host MXROOT=$(CURDIR) SUDO=

test: filesystem  src/usersim/sim
	src/usersim/sim

#
# The native self-compile, timed.  /usr/src inside the simulator's
# filesystem is rebuilt by the guest's own make and ccc: build and
# install, clobber, build a second generation, then verify it against
# what the first installed.  It is the build stage of the controller
# regression matrix (disks/hdinstall/matrix_test) without a disk or a
# boot - usersim hosts the same commands against filesystem/.
#
# The recipe prepares what the guest tree needs first (the cross
# bootstrap that supplies the seed compiler, and the source staging the
# host filesystem target does not do), so this runs from a clean tree.
#
SELFCOMPILE = cd /usr/src ; make ; make install ; make clobber ; make ; make verify

.PHONY: selfcompile
selfcompile: filesystem src/usersim/sim
	$(MAKE) -C src/micronix bootstrap
	$(MAKE) -C src/micronix update ROOT=$(abspath filesystem)
	@start=$$(date +%s); \
	src/usersim/sim -d filesystem -r /bin/sh -c '$(SELFCOMPILE)'; \
	rc=$$?; \
	echo "=== selfcompile: $$(( $$(date +%s) - start ))s, exit $$rc"; \
	exit $$rc

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
	rm -rf filesystem bin libexec ccc lib usr sim man

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

bootdisks: filesystem src/tools/mnix src/tools/setdev toolchain
	$(MAKE) -C src/micronix/stand/boot djbootimgs bootimgs
	for i in $(BOOTDISKS) ; do \
		disks/bootdisks/create_bootdisk $$i || exit 1 ; \
	done
