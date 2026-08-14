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
# vim: tabstop=4 shiftwidth=4 noexpandtab:

HDRS = 8080.h alloc.h bios.h dirent.h dtab.h dump.h fio.h mtab.h \
	obj.h pascal.h pwd.h std.h types.h utmp.h

all:

install:
	cd sys ; make install
	mkdir /usr/include
	cp $(HDRS) /usr/include

clean:

clobber:
