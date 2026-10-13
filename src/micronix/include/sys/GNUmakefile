#
# cross build for the kernel headers
#
# include/sys/GNUmakefile
#
# These are the system's own - inode, proc, file, buf, the device
# tables - and unlike the set one directory up, ccc owns none of
# them.  All of them install.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/include/sys
INCDIR	 = $(ROOT)/usr/include/sys
NOWARN	 = --warning=no-ignore-newer

HDRS	   = access.h buf.h con.h dir.h disk2.h disk3.h dj.h dlabel.h \
	 file.h fs.h hdca.h ide.h inode.h ints.h ioctl.h lock.h mount.h mw.h \
	 ncr.h ovl.h proc.h reboot.h reg.h sgtty.h signal.h stat.h sys.h \
	 times.h tty.h
MKFILE	 = $(wildcard ?akefile)

.PHONY: all update install install-man clean clobber doc

all:

sys.tar: $(HDRS) $(MKFILE)
	tar -f sys.tar -c $(MKFILE) $(HDRS)

update: sys.tar
	mkdir -p $(DEST)
	tar -f sys.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	mkdir -p $(INCDIR)
	cp $(HDRS) $(INCDIR)

install-man:

clean:

clobber: clean
	rm -f sys.tar

doc:
