#
# cross build for the libraries
#
# lib/GNUmakefile
#
# libunix was missing from DIRS, so the tree build never produced libunix.a
# and it only ever got built by hand in its own directory.  The order
# matters no more than it does for any archive, but libunix is the system
# call layer libc sits on, so it goes first.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = libunix libc libutil libccc fslib

RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

#
# This directory has a native makefile of its own - the one that runs
# "make install" in each library in turn - and the recursion alone
# would leave /usr/src/lib holding the subdirectories and nothing to
# drive them.  So update ships it, the way cmd does, and with
# --keep-newer-files for the same reason: a makefile edited inside
# micronix is not to be overwritten by an older one from here.
#
ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/lib
NOWARN	 = --warning=no-ignore-newer

.PHONY: all update install install-man clean clobber doc

all:
	$(RECURSE)

lib.tar: makefile
	tar -f lib.tar -c makefile

#
# The C library's own headers, lib/include.  Not a build directory - it
# has no makefile of its own - so the recursion above never reaches it,
# and without this the source tree is missing /usr/src/lib/include: a
# from-scratch native rebuild cannot find stdio.h.  These are the modern
# set, the ones lib/libc and lib/libunix are compiled against, whose FILE
# agrees with the library - distinct from include/ at the top, which is
# the old Whitesmith set.  Shipped the same way makefile is.
#
LIBHDRS	   = include/assert.h include/conio.h include/cpm.h \
	 include/ctype.h include/errno.h include/exec.h include/fcntl.h \
	 include/float.h include/hitech.h include/libgen.h include/limits.h \
	 include/math.h include/setjmp.h include/signal.h include/stat.h \
	 include/stdarg.h include/stddef.h include/stdio.h include/stdlib.h \
	 include/string.h include/sys.h include/time.h include/unistd.h \
	 include/unixio.h
LIBSYSHDRS	   = include/sys/wait.h

libinclude.tar: $(LIBHDRS) $(LIBSYSHDRS)
	tar -f libinclude.tar -c $(LIBHDRS) $(LIBSYSHDRS)

update: lib.tar libinclude.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f lib.tar -C $(DEST) --keep-newer-files -x $(NOWARN)
	tar -f libinclude.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

#
# This used to say that the cross build does not install these -
# that it links them into the commands it builds and putting them on
# the running system is the native install's job.  That was a circle:
# the native build cannot link anything until /lib has libc.a in it,
# and it was the native build that was supposed to put it there.  The
# archives install now, from GNUmakefile.inc, and lib/libc/makefile
# and its neighbours still install them again when the machine
# rebuilds itself.
#
install:
	$(RECURSE)

#
# Nothing of its own here; this exists so that the rule reaches the
# directories below, which do have pages.
#
install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f lib.tar libinclude.tar

doc:
	$(RECURSE)
