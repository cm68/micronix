#
# install the system headers, on micronix
#
# include/makefile
#
# The native build.  GNUmakefile beside this one is the cross build and
# says at length why six headers are not copied; the short version is
# that ccc owns cpm.h, errno.h, hitech.h, setjmp.h, stdio.h and time.h,
# libc.a is compiled against ccc's stdio.h, and installing ours over it
# would give every command a FILE the library disagrees with.
#
# So the list is written out.  This make has no wildcard and no
# filter-out, and "cp *.h /usr/include" - which is what the Makefile
# here used to do - copies all twenty, ccc's six included.
#
# /include is the old Whitesmiths set and is not touched.
#
# Withholding ccc's six is only half of it, and the other half was
# missing: the headers that belong under those names have to be put
# there, and they live in ../lib/include - the C library's own set, the
# ones ccc installs under those names and the ones libc.a was compiled
# against.  Nothing installed them.  lib/include is not in lib's
# recursion and has no makefile of its own, and /usr/include is written
# here and nowhere else, so a from-scratch native build stopped at the
# first file that opens stdio.h.  They go on AFTER $(HDRS): six names
# are in both sets and the library's are the ones that count.
#
# ../lib/include/sys holds wait.h, and it installs with them.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

HDRS = 8080.h alloc.h bios.h dirent.h dtab.h dump.h fio.h fslib.h \
	mtab.h obj.h pascal.h pwd.h std.h types.h utmp.h

#
# The C library's headers, in ../lib/include.  Written out for the same
# reason $(HDRS) is.
#
# They are copied by cd'ing into that directory rather than by putting
# it in front of $(LIBHDRS) on the cp line.  A macro is text: the
# expansion of "../lib/include/$(LIBHDRS)" is "../lib/include/assert.h"
# followed by twenty-three bare names, and only the first one gets the
# directory - cp then reports "conio.h: No such file or directory" for
# each of the rest.  Each recipe line is its own shell, so the cd is
# not carried into the next one; "../" is right at the start of each.
#
LIBHDRS = assert.h conio.h cpm.h ctype.h errno.h exec.h fcntl.h \
	float.h hitech.h libgen.h limits.h math.h setjmp.h signal.h \
	stat.h stdarg.h stddef.h stdio.h stdlib.h string.h sys.h \
	time.h unistd.h unixio.h

LIBSYSHDRS = wait.h

all:

install:
	cd sys ; make install
	mkdir -p /usr/include/sys
	cp $(HDRS) /usr/include
	cd ../lib/include ; cp $(LIBHDRS) /usr/include
	cd ../lib/include/sys ; cp $(LIBSYSHDRS) /usr/include/sys

clean:

clobber:
