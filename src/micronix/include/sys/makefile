#
# install the kernel headers, on micronix
#
# include/sys/makefile
#
# All of these install: ccc owns none of them.  "cp *.h" would do here
# and the list is written out anyway, so that this file reads the same
# way as the one above it and so that a header added to the directory
# has to be added here too - which is the point at which somebody
# notices it exists.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

HDRS = access.h buf.h con.h dir.h dj.h dlabel.h file.h fs.h ide.h inode.h \
	lock.h mount.h mw.h ovl.h proc.h reboot.h reg.h sgtty.h signal.h stat.h \
	sys.h tty.h

all:

install:
	mkdir -p /usr/include/sys
	cp $(HDRS) /usr/include/sys

clean:

clobber:

#
# verify - the inverse of install: every installed artifact against
# the one this build just produced.  Run it after a clobber and a
# second build to prove the rebuild matches what the first
# generation installed.
#
verify:
	cmp access.h /usr/include/sys/access.h
	cmp buf.h /usr/include/sys/buf.h
	cmp con.h /usr/include/sys/con.h
	cmp dir.h /usr/include/sys/dir.h
	cmp dj.h /usr/include/sys/dj.h
	cmp dlabel.h /usr/include/sys/dlabel.h
	cmp file.h /usr/include/sys/file.h
	cmp fs.h /usr/include/sys/fs.h
	cmp ide.h /usr/include/sys/ide.h
	cmp inode.h /usr/include/sys/inode.h
	cmp lock.h /usr/include/sys/lock.h
	cmp mount.h /usr/include/sys/mount.h
	cmp mw.h /usr/include/sys/mw.h
	cmp ovl.h /usr/include/sys/ovl.h
	cmp proc.h /usr/include/sys/proc.h
	cmp reboot.h /usr/include/sys/reboot.h
	cmp reg.h /usr/include/sys/reg.h
	cmp sgtty.h /usr/include/sys/sgtty.h
	cmp signal.h /usr/include/sys/signal.h
	cmp stat.h /usr/include/sys/stat.h
	cmp sys.h /usr/include/sys/sys.h
	cmp tty.h /usr/include/sys/tty.h
