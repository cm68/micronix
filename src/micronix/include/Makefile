#
# include directory in the source tree
#

INCLUDES = \
	8080.h alloc.h bios.h cpm.h fio.h dtab.h dump.h errno.h obj.h \
	mtab.h pascal.h pwd.h setjmp.h std.h stdio.h time.h types.h utmp.h \
	dirent.h

all:

install:
	cp $(INCLUDES) /include
	cd sys ; make install
