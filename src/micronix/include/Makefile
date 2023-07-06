#
# include file installation to filesystem root
# after this file is done, the include tree is rationalized
#
# /include/Makefile
#
# Changed: <2023-07-04 11:03:50 curt>

#
# include files that have been revised or moved
# we save these to /old
#
REPLACE = dir.h fs.h sgtty.h stat.h access.h signal.h

INCLUDES = \
	8080.h alloc.h bios.h cpm.h fio.h dtab.h dump.h errno.h obj.h \
	mtab.h pascal.h pwd.h setjmp.h std.h stdio.h time.h types.h utmp.h \
	dirent.h

all:

upgrade:
	mkdir /old/include
	cd /include ; mv $(REPLACE) /old/include
	cp $(INCLUDES) /include
	cd sys ; make install

install:
	cp $(INCLUDES) /include
	cd sys ; make install

# vim: tabstop=4 shiftwidth=4 expandtab:

