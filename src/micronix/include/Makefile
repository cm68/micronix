#
# include file installation to filesystem root
# after this file is done, the include tree is rationalized
#
# include/Makefile
# Changed: <2022-01-06 15:52:30 curt>

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
