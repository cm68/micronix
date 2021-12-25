#
# Makefile to install the include/sys header files
#
# include/sys/Makefile 
# Changed: <2021-12-23 14:17:45 curt>
#

INCLUDES = \
	access.h buf.h con.h dir.h file.h fs.h inode.h lock.h mount.h \
	proc.h reg.h sgtty.h signal.h stat.h sys.h tty.h

all:

install:
	mkdir /include/sys
	cp $(INCLUDES) /include/sys

# vim: tabstop=4 shiftwidth=4 expandtab:
