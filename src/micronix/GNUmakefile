#
# top of the micronix cross build
#
# /GNUmakefile
#
# This drives the cross build only.  The "makefile" beside it is the
# native build that runs inside the simulator.
#
# hitech and whitesmith are deliberately not here.  Neither is part of
# the build any more: the compiler that builds this tree is ccc, built
# into $(BIN) by "make hostcc".  Their sources are still in the tree,
# with their own makefiles, and are built by hand if they are built at
# all.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# include is here now.  It was not, and nothing in the cross build
# looked at that directory at all: the headers in /usr/src/include
# were whatever had been copied there in 2023, and four of them had
# drifted from the tree.  It goes FIRST - what everything else
# compiles against is installed before anything compiles.
#
#
# etc is here for the same reason and after the same gap: it is a
# directory of plain files that the simulated filesystem's /etc is
# built from, nothing in the cross build looked at it, and a file added
# to it did not reach the machine.  See its GNUmakefile - it is a
# source directory of seeds and is installed by the cross build only.
#
DIRS = include etc lib libexec cmd sys stand cpm

#
# The top GNUmakefile is a leaf like the rest: DEPTH is the distance to
# GNUmakefile.inc, which sits here, so it is ".".  Everything shared
# comes from there - the compiler and librarian, the install/clean/
# update rules, and install-hwsim/install-usersim for the simulators.
# What stays below is only what is top-specific: the host tools built
# here rather than in a DIR, and the header install that needs sudo.
#
DEPTH = .

#
# Put the system headers where the compiler looks for them.
#
# ccc resolves <sys/...> out of its system include directory - libdir
# up one and into include, see cmd/ccc/ccc.c - and that search runs
# before -I.  These are the headers that directory should be holding:
# sys/stat.h, sys/fs.h, sys/dir.h, types.h and the rest are the
# system's, and the system is this tree.  Until they are installed,
# every command that stats a file has to be told where to look.
#
# sudo, because $(PREFIX) is not ours.  SUDO= runs it without, for a
# DESTDIR you already own.
#
PREFIX	?= /usr/local
SYSINC	 = $(DESTDIR)$(PREFIX)/ccc/include
SUDO	?= sudo

#
# NOT copied: the headers ccc installs under these names.  A command
# links the libc built in lib/libc, and that libc is compiled against
# ccc's stdio.h; this tree's stdio.h is the old Whitesmith one and its
# FILE is a different shape.  Since the system include directory is
# searched first, copying ours over ccc's would not fail to build - it
# would give every command a FILE that disagrees with the library
# implementing it, and corrupt buffers at run time.  They are the C
# library's interface, and the C library owns them.
#
# These stay in the tree because the native build inside the simulator
# uses them, out of /include, against the compiler that matches them.
#
CCCOWNED = cpm.h errno.h hitech.h setjmp.h stdio.h time.h
SYSHDRS	 = 8080.h alloc.h bios.h crypt.h dirent.h dtab.h dump.h fio.h \
	 fslib.h grp.h mtab.h obj.h pascal.h pwd.h std.h types.h utmp.h

include $(DEPTH)/GNUmakefile.inc

.PHONY: hostcc hostrt bootstrap sysinstall sysrt install-man-host install-host footprint

#
# The librarian, built for the HOST.
#
# The archives this tree makes are read by a linker that also runs on
# the host, so the program that writes them has to be a host binary -
# and cmd/ar builds the z80 one, which cannot.  Same source, two
# targets.
#
# It is called mxar and not ar because the system's ar would shadow
# the one that reads micronix archives for everything on this machine.
# The "mx" prefix is what this tree uses for a host program that speaks
# micronix - see mxccc and mxnm - and NOT the ".mx" suffix, which means
# a micronix executable, which this is not: it is an ELF binary that
# reads and writes micronix archives.
#
# It is built HERE and not in cmd/ar because DIRS is "include lib
# libexec cmd sys", and the libraries are archived before cmd is
# reached.  The symbol lister is the same shape: cmd/nm builds the z80
# one, and this builds mxnm beside mxar, in $(BIN).
#
ARMX	 = $(BIN)/mxar
ARSRC	 = cmd/ar/ar.c
NMX	 = $(BIN)/mxnm
NMSRC	 = cmd/nm/nm.c lib/libutil/wsobj.c
SETDEVX	 = $(BIN)/setdev
SETDEVSRC = cmd/setdev/setdev.c
#
# lz4, the same shape again: cmd/lz4 builds the z80 one for the guest,
# and this builds the host one.  Both are needed by the same thing - tar
# z compresses and decompresses through whichever of the pair is local
# - so the distribution packer, which runs here, wants mxlz4 and the
# guest wants lz4.  They speak one stream format, the LZ4 frame
# format with the history window narrowed to 16K, so a stream written
# by either is read by the other.
#
# mxunlz4 is the reverse and is built beside it: not because anything
# on the host decompresses in anger, but because it is the only way to
# check a stream the packer wrote without booting a machine to do it.
#
MXLZ4	 = $(BIN)/mxlz4
MXLZ4SRC = cmd/lz4/lz4.c
MXUNLZ4	 = $(BIN)/mxunlz4
MXUNLZ4SRC = cmd/unlz4/unlz4.c
#
# The distribution packer, the same shape a third time: cmd/packsrc
# builds the z80 one for the guest, and this builds the host one that
# create_dist runs.  It reads a directory tree and writes tar files
# right-sized to a volume, and it is in C rather than shell and awk so
# that it needs nothing the tree does not build itself - no sort, no
# find -printf, no gawk, and the same source runs on micronix.
#
MXPAKSRC = $(BIN)/packsrc
MXPAKSRCSRC = cmd/packsrc/packsrc.c
#
# And the tar the packer's archives have to be readable by, built for the
# host for the same reason: the distribution is put together here, and
# the thing that reads an archive on the other end is cmd/tar.  A host
# tar means an archive can be checked against the reader that will
# actually open it, instead of against the system's tar - which is a
# stricter checker and a different format, and so a good second opinion
# but not the one that decides.
#
# Both tars now speak the same bytes: packsrc's headers are spelled the
# way cmd/tar/tar.c spells them, so an archive from either compares equal
# to the other but for the owner.
#
MXTAR	 = $(BIN)/mxtar
MXTARSRC = cmd/tar/tar.c
HOSTCC	?= gcc
#
# -idirafter, not -I: include/ holds the old Whitesmith stdio.h and the
# v6 sys/*.h, and putting it on the -I path would shadow the host's own.
# obj.h is not a host header, so it falls through to include/.
#
HOSTCFLAGS ?= -w -std=gnu89 -Dlinux -idirafter $(DEPTH)/include

#
# The host tools are prerequisites of all, not part of the recursion,
# so the librarian exists before lib is archived.  This is a
# prerequisite line only - the recipe is GNUmakefile.inc's.
#
all: $(ARMX) $(NMX) $(SETDEVX) $(MXLZ4) $(MXUNLZ4) $(MXPAKSRC) $(MXTAR)

#
# clobber removes the host tools and src.tar as well as what the
# recursion removes.  GNUmakefile.inc's clobber reads GENERATED.
#
GENERATED = src.tar srctar.tar $(ARMX) $(NMX) $(SETDEVX) $(MXLZ4) $(MXUNLZ4) $(MXPAKSRC) $(MXTAR)

$(ARMX): $(ARSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(ARSRC)

#
# The host symbol lister.  nm on micronix, mx-prefixed here like mxar
# and mxccc: it reads micronix objects, which the system nm cannot.
# Built from the same sources as cmd/nm, with the machine's own cc.
#
$(NMX): $(NMSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(NMSRC)

$(SETDEVX): $(SETDEVSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(SETDEVSRC)

#
# The compressor and its reverse, for the host.  One source each, and
# neither reads a micronix object, so they want no library - the plain
# HOSTCFLAGS rule the three above use, and not host.mk, which exists to
# link the compiler's passes against libccc.
#
$(MXLZ4): $(MXLZ4SRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(MXLZ4SRC)

$(MXUNLZ4): $(MXUNLZ4SRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(MXUNLZ4SRC)

$(MXPAKSRC): $(MXPAKSRCSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(MXPAKSRCSRC)

$(MXTAR): $(MXTARSRC)
	mkdir -p $(BIN)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $(MXTARSRC)

#
# The cross compiler, built here from these sources: the seven passes
# and the driver, with the machine's own cc, into $(BIN) and
# $(HOSTROOT)/libexec.
#
# Then the runtime beside it, because the driver adds crt0.o and
# libc.a from its own lib directory whatever else is on the link line,
# and works that directory out from argv[0] - $(BIN)/mxccc reads
# $(HOSTROOT)/ccc/lib and $(HOSTROOT)/ccc/include.  The archives have to
# exist before anything links, which is why this is a target you ask
# for and not part of all: the libraries are built by the compiler it
# installs.
#
#	make hostcc		build the compiler
#	make			build the tree with it, once lib is made
#	make hostrt		put the runtime where it can find it
#
HOSTCCDIRS = libexec/pass0 libexec/c0 libexec/c1 libexec/peep \
	     cmd/asz cmd/ld libexec/astpp cmd/ccc

hostcc: $(ARMX)
	@for d in $(HOSTCCDIRS); do $(MAKE) -C $$d host || exit 1; done
	@echo "host compiler in $(BIN)/mxccc"

hostrt:
	@mkdir -p $(HOSTROOT)/ccc/lib $(HOSTROOT)/ccc/include/sys $(HOSTROOT)/ccc/include/cpm
	cp lib/libc/libc.a lib/libunix/libunix.a lib/libunix/crt0.o \
	   lib/libccc/libccc.a $(HOSTROOT)/ccc/lib
	#
	# No micronix headers go here any more, and that is the change.
	# The compiler resolves <sys/...> out of this directory when a
	# build does not say otherwise, so filling it meant a second
	# copy of the tree's headers, refreshed whenever somebody
	# remembered to run this.  It cost a day: mkfs wrote s_time at
	# one offset and the kernel read it at another, out of two
	# vintages of sys/fs.h.  GNUmakefile.inc sets -i to the tree's
	# lib/include now, so no build reads a copy and there is nothing
	# here to keep in step.  Taken out rather than left, so a build
	# that does depend on the default directory stops at the include
	# and says so, instead of compiling against something stale.
	#
	rm -f $(HOSTROOT)/ccc/include/*.h
	rm -f $(HOSTROOT)/ccc/include/sys/*.h
	#
	# CP/M keeps a header tree here, and the reason is FILE.  Its
	# stdio.h sits over fcb's and has different flag bits, so it
	# cannot share the micronix one.  Nothing in the tree builds
	# -m cpm with an -i of its own - stand/formatmw is the one that
	# does - which is why this stays.
	#
	# The C library's headers, with the CP/M ones over them.  cp
	# onto a link would write through it, into the tree, so -f
	# unlinks first.
	#
	@for f in lib/include/*.h lib/include/sys/*.h cpm/include/*.h; do \
		ln -sf ../../../src/micronix/$$f \
			$(HOSTROOT)/ccc/include/cpm/$${f##*/}; \
	done
	@echo "runtime in $(HOSTROOT)/ccc/lib"

#
# The same runtime, for the installed host compiler: under $(PREFIX)/ccc
# rather than the tree, with sudo because $(PREFIX) is not ours.  The
# layout is hostrt's, only the root differs; the driver reads it out of
# /usr/local/ccc when run from /usr/local/bin (LIBSUF in ccc.c), which
# keeps this tree's runtime out of the /usr/local/lib and
# /usr/local/usr/include an installed cc looks in.
#
sysrt:
	$(SUDO) mkdir -p $(PREFIX)/ccc/include/sys $(PREFIX)/ccc/include/cpm
	$(SUDO) cp lib/libc/libc.a lib/libunix/libunix.a lib/libunix/crt0.o \
	   lib/libccc/libccc.a $(PREFIX)/ccc/lib
	$(SUDO) cp include/*.h $(PREFIX)/ccc/include
	$(SUDO) cp include/sys/*.h $(PREFIX)/ccc/include/sys
	$(SUDO) cp lib/include/*.h $(PREFIX)/ccc/include
	$(SUDO) cp lib/include/sys/*.h $(PREFIX)/ccc/include/sys
	$(SUDO) cp lib/include/*.h $(PREFIX)/ccc/include/cpm
	$(SUDO) cp lib/include/sys/*.h $(PREFIX)/ccc/include/cpm
	$(SUDO) cp cpm/include/*.h $(PREFIX)/ccc/include/cpm
	#
	# sudo runs under the root umask (0077 on this machine), so the cp
	# above left every file 0600 and unreadable by whoever runs the
	# compiler.  Make the installed tree readable.
	#
	$(SUDO) chmod -R a+rX $(PREFIX)/ccc
	@echo "runtime in $(PREFIX)/ccc/lib"

# The bootstrap: build the cross compiler, the host tools and the boot
# images the build runs (setdev stamps the kernel, mxnm lists symbols,
# mxlz4/mxunlz4 and mxtar pack the distribution, bootimg is what the rom
# reads), build the runtime it links against, then cross-build every Z80
# binary and install it into the filesystem the simulator boots.  That
# filesystem is the seed the native build - "cd /usr/src ; make install"
# - rebuilds from.
#
# The host tools go into $(BIN), the tree's own bin/ - not /usr/local -
# so a bootstrap needs nothing installed outside the tree.
#
# The order is the whole of it, the INSTALLATION.md recipe as one target:
#
#	hostcc		the cross compiler (mxccc) and its passes, and mxar
#	lib		libc, libunix, libccc, crt0 - built with mxccc
#	hostrt		those archives where mxccc looks for them
#	all		the host tools, the boot images, every Z80 binary
#	install		every Z80 binary, into the filesystem
#
bootstrap: hostcc
	$(MAKE) -C lib
	$(MAKE) hostrt
	$(MAKE) all
	$(MAKE) install

sysinstall:
	$(SUDO) mkdir -p $(SYSINC)/sys
	$(SUDO) install -m 644 include/sys/*.h $(SYSINC)/sys
	(cd include && $(SUDO) install -m 644 $(SYSHDRS) $(SYSINC))
	@echo "installed: $(SYSINC)/sys, and $(words $(SYSHDRS)) headers beside it"
	@echo "not installed, the C library owns them: $(CCCOWNED)"

#
# Every man page in the tree, mirrored onto the host under $(MXMAN) so
# mxman can read them without booting anything.  The mx* tools are in
# $(PREFIX)/bin and this is their page tree, one directory per section
# like /usr/man.
#
# These are NOT the pages the system installs.  What installs is one
# page per command beside its source, plus libunix's section 2 and libc's
# section 3, and that is what lands in the simulator's /usr/man.  This
# is a superset: it carries the recovered 1.61 pages and the 1.3 ones
# off the floppy as well, including the pages for commands this tree
# has not built yet (sort, wall and the rest), which are the
# reason for having it - they are readable nowhere else.
#
# Listed rather than globbed together because the order IS the
# precedence: where two directories hold a page of the same name, the
# first one read is the one that lands.  Nothing collides today.  The
# names that did - sh, df, label, login, mount, owner, umount and ps,
# each a command this tree has ported whose page was already in the
# man tree - were settled by which of the two copies had something the
# other did not.  Where the original differed from the page beside the
# source it now carries an old- name, man/man1's oldsh.1, oldowner.1
# and oldps.1, named for what each documents, so both pages ship and
# neither has to win.  Where the two were byte for byte the same page
# - df, label, login, mount, umount - the man tree's copy was dropped
# instead, the page beside the source being the one that installs.
# That makes the rule a guard rather than a decision, and it reports
# what it shadowed when it shadows anything.
#
MXROOT	 = $(DESTDIR)$(PREFIX)/micronix
MXMAN	 = $(MXROOT)/man
HOSTMANS	 = cmd/ar/ar.1 cmd/asz/asz.1 cmd/awk/awk.1 cmd/cal/cal.1 \
	 cmd/cat/cat.1 cmd/cc/cc.1 cmd/ccc/ccc.1 cmd/change/change.1 \
	 cmd/chgrp/chgrp.1 cmd/chmod/chmod.1 cmd/chown/chown.1 \
	 cmd/clean/clean.1 cmd/cmp/cmp.1 cmd/comm/comm.1 cmd/common/common.1 \
	 cmd/compare/compare.1 cmd/compress/compress.1 cmd/concat/concat.1 \
	 cmd/copy/copy.1 cmd/cp/cp.1 cmd/cpp/cpp.1 cmd/crypt/crypt.1 \
	 cmd/cu/cu.1 cmd/date/date.1 cmd/dc/dc.1 cmd/dcheck/dcheck.1 \
	 cmd/dd/dd.1 cmd/detab/detab.1 cmd/df/df.1 cmd/diff/diff.1 \
	 cmd/djformat/djformat.1 cmd/down/down.1 cmd/du/du.1 cmd/echo/echo.1 \
	 cmd/ed/ed.1 cmd/edit/edit.1 cmd/entab/entab.1 cmd/expand/expand.1 \
	 cmd/expr/expr.1 cmd/far/far.1 cmd/fd2/fd2.1 cmd/fdj/fdj.1 \
	 cmd/fgrep/fgrep.1 cmd/field/field.1 cmd/file/file.1 cmd/find/find.1 \
	 cmd/form/form.1 cmd/fp/fp.1 cmd/fsck/fsck.1 cmd/grep/grep.1 \
	 cmd/group/group.1 cmd/halt/halt.1 cmd/hdformat/hdformat.1 \
	 cmd/icheck/icheck.1 cmd/include/include.1 cmd/init/init.1 \
	 cmd/kill/kill.1 cmd/kstat/kstat.1 cmd/kwic/kwic.1 cmd/label/label.1 \
	 cmd/last/last.1 cmd/ld/ld.1 cmd/less/less.1 cmd/lex/lex.1 cmd/ln/ln.1 \
	 cmd/login/login.1 cmd/ls/ls.1 cmd/lz4/lz4.1 cmd/m4/m4.1 \
	 cmd/macro/macro.1 cmd/mail/mail.1 cmd/make/make.1 \
	 cmd/makekey/makekey.1 cmd/man/help.1 cmd/man/man.1 cmd/md/md.1 \
	 cmd/mkdir/mkdir.1 cmd/mkfs/mkfs.1 cmd/mknod/mknod.1 cmd/more/more.1 \
	 cmd/mount/mount.1 cmd/msh/msh.1 cmd/mv/mv.1 cmd/mwformat/mwformat.1 \
	 cmd/ncheck/ncheck.1 cmd/newuser/newuser.1 cmd/nm/nm.1 cmd/od/od.1 \
	 cmd/overstrike/overstrike.1 cmd/owner/owner.1 cmd/packsrc/packsrc.1 \
	 cmd/passwd/passwd.1 cmd/pr/pr.1 cmd/print/print.1 cmd/ps/ps.1 \
	 cmd/pwd/pwd.1 cmd/ratfor/ratfor.1 cmd/rm/rm.1 cmd/rmdir/rmdir.1 \
	 cmd/s/s.1 cmd/scsiformat/scsiformat.1 cmd/sed/sed.1 \
	 cmd/setdev/setdev.1 cmd/sh/sh.1 cmd/size/size.1 cmd/sort/sort.1 \
	 cmd/split/split.1 cmd/strings/strings.1 cmd/strip/strip.1 \
	 cmd/stty/stty.1 cmd/su/su.1 cmd/sum/sum.1 cmd/sync/sync.1 \
	 cmd/tail/tail.1 cmd/tar/tar.1 cmd/td/td.1 cmd/tee/tee.1 \
	 cmd/time/time.1 cmd/touch/touch.1 cmd/tr/tr.1 cmd/translit/translit.1 \
	 cmd/tree/tree.1 cmd/tty/tty.1 cmd/umount/umount.1 cmd/unique/unique.1 \
	 cmd/unlz4/unlz4.1 cmd/unrot/unrot.1 cmd/update/update.1 cmd/upm/upm.1 \
	 cmd/uudecode/uudecode.1 cmd/uuencode/uuencode.1 cmd/vi/vi.1 \
	 cmd/wall/wall.1 cmd/wc/chars.1 cmd/wc/lines.1 cmd/wc/wc.1 \
	 cmd/wc/words.1 cmd/who/who.1 cmd/write/write.1 cmd/xargs/xargs.1 \
	 cmd/yacc/yacc.1 lib/libunix/Intro.2 lib/libunix/access.2 \
	 lib/libunix/break.2 lib/libunix/chdir.2 lib/libunix/chmod.2 \
	 lib/libunix/chown.2 lib/libunix/close.2 lib/libunix/creat.2 \
	 lib/libunix/csw.2 lib/libunix/dup.2 lib/libunix/exec.2 \
	 lib/libunix/exit.2 lib/libunix/fork.2 lib/libunix/fstat.2 \
	 lib/libunix/ftruncate.2 lib/libunix/getpid.2 lib/libunix/getuid.2 \
	 lib/libunix/gtty.2 lib/libunix/indir.2 lib/libunix/kill.2 \
	 lib/libunix/link.2 lib/libunix/mknod.2 lib/libunix/mount.2 \
	 lib/libunix/nice.2 lib/libunix/open.2 lib/libunix/pipe.2 \
	 lib/libunix/read.2 lib/libunix/seek.2 lib/libunix/setuid.2 \
	 lib/libunix/signal.2 lib/libunix/sleep.2 lib/libunix/stat.2 \
	 lib/libunix/stime.2 lib/libunix/stty.2 lib/libunix/sync.2 \
	 lib/libunix/time.2 lib/libunix/times.2 lib/libunix/umount.2 \
	 lib/libunix/unlink.2 lib/libunix/wait.2 lib/libunix/write.2 \
	 lib/libc/alloc.3 lib/libc/crypt.3 lib/libc/ctime.3 lib/libc/fclose.3 \
	 lib/libc/ferror.3 lib/libc/floor.3 lib/libc/fopen.3 lib/libc/fread.3 \
	 lib/libc/fseek.3 lib/libc/getgrent.3 lib/libc/getpass.3 \
	 lib/libc/getpwent.3 lib/libc/gets.3 lib/libc/perror.3 lib/libc/popen.3 \
	 lib/libc/printf.3 lib/libc/putc.3 lib/libc/puts.3 lib/libc/scanf.3 \
	 lib/libc/setbuf.3 lib/libc/stdio.3 lib/libc/strcat.3 lib/libc/strchr.3 \
	 lib/libc/strcmp.3 lib/libc/strcpy.3 lib/libc/strdup.3 \
	 lib/libc/strlen.3 lib/libc/strncat.3 lib/libc/strncmp.3 \
	 lib/libc/strncpy.3 lib/libc/strrchr.3 lib/libc/system.3 \
	 lib/libc/ttyname.3 man/man1/account.1 man/man1/anat.1 \
	 man/man1/archive.1 man/man1/as.1 man/man1/badspots.1 man/man1/boot.1 \
	 man/man1/cal.1 man/man1/chars.1 man/man1/clean.1 man/man1/comm.1 \
	 man/man1/cp1.1 man/man1/cp2.1 man/man1/cpp.1 man/man1/cptree.1 \
	 man/man1/crypt.1 man/man1/cu.1 man/man1/cxr.1 man/man1/dar.1 \
	 man/man1/dc.1 man/man1/ddt.1 man/man1/down.1 man/man1/edit.1 \
	 man/man1/field.1 man/man1/file.1 man/man1/group.1 man/man1/hex.1 \
	 man/man1/last.1 man/man1/lib.1 man/man1/lines.1 man/man1/link.1 \
	 man/man1/lord.1 man/man1/lpr.1 man/man1/macro.1 man/man1/mail.1 \
	 man/man1/msgs.1 man/man1/newuser.1 man/man1/obj.1 man/man1/oldcc.1 \
	 man/man1/oldowner.1 man/man1/oldps.1 man/man1/oldsh.1 \
	 man/man1/passwd.1 man/man1/paste.1 man/man1/pilot.1 man/man1/ptc.1 \
	 man/man1/rp.1 man/man1/sort.1 man/man1/split.1 man/man1/stty.1 \
	 man/man1/su.1 man/man1/sum.1 man/man1/tail.1 man/man1/touch.1 \
	 man/man1/translit.1 man/man1/tree.1 man/man1/tty.1 man/man1/unique.1 \
	 man/man1/unrot.1 man/man1/update.1 man/man1/wall.1 man/man1/who.1 \
	 man/man1/words.1 man/man2/alarm.2 man/man2/reboot.2 man/man4/cables.4 \
	 man/man4/djdma.4 man/man4/djmem.4 man/man4/hdca.4 man/man4/hddma.4 \
	 man/man4/ide.4 man/man4/io.4 man/man4/mem.4 man/man4/multio.4 \
	 man/man4/network.4 man/man4/null.4 man/man4/ports.4 \
	 man/man4/printers.4 man/man5/ascii.5 man/man5/banner.5 \
	 man/man5/device.5 man/man5/directory.5 man/man5/disklabel.5 \
	 man/man5/dtab.5 man/man5/filesystem.5 man/man5/fp.5 man/man5/group.5 \
	 man/man5/motd.5 man/man5/mtab.5 man/man5/passwd.5 man/man5/rc.5 \
	 man/man5/signon.5 man/man5/terminals.5 man/man5/ttys.5 man/man5/utmp.5 \
	 man/man5/wtmp.5

#
# The shadowing is worked out here, in the shell, and not by testing
# the target for the name already being there.  Testing does not work:
# $(SUDO) runs under the root umask, so the directory it just made is
# 0700 and a test by anyone else fails with EACCES rather than ENOENT -
# "no such file", so nothing is ever a duplicate, every page is copied,
# and the LAST one to claim a name is the one that lands - the reverse
# of the precedence the list above is written to express.  A name seen
# once is remembered instead, and the file is copied only if it has not
# been.
#
# The chmod at the end is not tidiness.  $(SUDO) runs under the root
# umask - 0077 on this machine - so every copy lands 0600 in a 0700
# directory and mxman, which does not run as root, could read none of
# it.  Same line sysrt ends with, for the same reason, and it has to
# start at $(MXROOT) rather than at the man directory: the directory
# above the pages is one of the 0700 ones, and a readable page inside
# an unsearchable directory is still unreadable.
#
install-man-host:
	@for s in 1 2 3 4 5 ; do $(SUDO) mkdir -p $(MXMAN)/man$$s ; done
	@seen=" " ; n=0 ; dup=0 ; for f in $(HOSTMANS) ; do \
		s=$${f##*.} ; b=$${f##*/} ; \
		case "$$seen" in *" $$b "*) \
			echo "  shadowed, another page has $$b: $$f" ; \
			dup=$$((dup+1)) ; continue ;; \
		esac ; \
		seen="$$seen$$b " ; \
		$(SUDO) cp $$f $(MXMAN)/man$$s/$$b ; n=$$((n+1)) ; \
	done ; \
	echo "$$n pages in $(MXMAN), $$dup shadowed in the tree"
	$(SUDO) chmod -R a+rX $(MXROOT)

#
# The cross man and formatter, built for the host and installed beside
# the man pages: mxman reads $(MXMAN) and execs mxform by name, so the
# two live in the same $(MXROOT)/bin directory.
#
install-host:
	$(MAKE) -C cmd/man host
	$(MAKE) -C cmd/form host
	$(SUDO) mkdir -p $(MXROOT)/bin
	$(SUDO) install -m 755 $(BIN)/mxman $(BIN)/mxform $(MXROOT)/bin/
	$(SUDO) chmod -R a+rX $(MXROOT)

#
# The top of the source tree the simulator sees.  The recursion fills
# in /usr/src/lib, /usr/src/cmd and the rest; this ships the makefile
# that drives them, which nothing did - /usr/src/makefile was a 2023
# copy that still had "cd sys" commented out and had never heard of
# libexec.  Beside it were a Makefile and a GNUmakefile from the same
# era, neither of which anything reads on that machine.
#
# --keep-newer-files, as everywhere else: a makefile edited inside
# micronix is not to be overwritten by an older one from here.
#
# This update is top-specific: it ships src.tar (the makefile above),
# which GNUmakefile.inc's update does not know about, so it overrides
# that rule.  It ships the prose too, and for the same reason: the
# project's .md notes live in the repository's top-level docs/, which is
# outside this directory and so outside $(DIRS) - the recursion cannot
# reach it and a directory it cannot reach is a directory that does not
# ship.  They land at /usr/src/docs, which is where the comments that
# point at them say they are.
#
ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src
NOWARN	 = --warning=no-ignore-newer
DOCS	 = ../../docs
DOCSRCS	 = ../../docs/INTERRUPTS.md ../../docs/WORKORDER-provisioning.md
src.tar: makefile
	tar -f src.tar -c makefile

update: src.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f src.tar -C $(DEST) --keep-newer-files -x $(NOWARN)
	@if [ -n "$(DOCSRCS)" ]; then \
		mkdir -p $(DEST)/docs; \
		cp $(DOCSRCS) $(DEST)/docs; \
	fi

#
# One tar of the whole source tree - sources only, no .o or built
# binaries - for create_vol to ship onto a volume.  It reuses update,
# which stages exactly the sources (SRCS) per directory, but points it
# at a scratch tree so the tar is not polluted by whatever an earlier
# update left in the filesystem's /usr/src.  create_vol puts it on the
# volume with "mnix tar x -C /usr/src".
#
SRCTREE = srctree
SRCTAR = srctar.tar

#
# The files, listed, and not the directory they live in.  "tar -c ."
# archives every entry it finds, which is not the same set as the one
# update just staged: it is whatever is in the directory, and it puts a
# member in the archive for each directory as well.  Naming the files
# makes the archive the staged set and nothing else.
#
# The list is built after the staging, from the staged tree, rather
# than from SRCS - the recursion is what knows which files, per
# directory, and asking it twice would be a second answer to the same
# question.  Names go in without the leading "./" that find writes, so
# the archive reads as "cmd/ar/ar.c" and matches what the
# per-directory tars carry.
#
# mnix tar x makes the path to each file it writes, so an archive of
# files alone extracts whole.  See tarx in src/tools/mnix.c.
#
.PHONY: tarfile
tarfile:
	rm -rf $(SRCTREE)
	$(MAKE) update ROOT=$(abspath $(SRCTREE))
	cd $(abspath $(SRCTREE))/usr/src && \
		find . -type f -print | sed 's|^\./||' > $(abspath $(SRCTAR)).list && \
		tar -f $(abspath $(SRCTAR)) -c -T $(abspath $(SRCTAR)).list
	rm -f $(abspath $(SRCTAR)).list
	rm -rf $(SRCTREE)

#
# The tipping-point guard (see libexec/GNUmakefile): can the compiler
# compile itself on the user-mode simulator?  Fails on out-of-memory,
# a gap below FLOOR (default 256), or divergence from the host.
#
footprint:
	$(MAKE) -C libexec footprint
