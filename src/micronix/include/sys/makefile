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

HDRS = access.h buf.h con.h dir.h dj.h dlabel.h file.h fs.h inode.h \
	lock.h mount.h mw.h proc.h reg.h sgtty.h signal.h stat.h \
	sys.h tty.h

all:

install:
	mkdir /usr/include/sys
	cp $(HDRS) /usr/include/sys

clean:

clobber:
