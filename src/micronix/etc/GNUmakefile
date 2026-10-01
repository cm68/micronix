#
# cross build for the system's own files
#
# etc/GNUmakefile
#
# There was no GNUmakefile here and etc was not in the top level's
# DIRS, so the cross build never looked at this directory: what was in
# the simulated filesystem's /etc was whatever had been copied there
# once by hand, and a file added here - rc, most recently - did not
# reach the machine at all.
#
# It is the same shape of gap the headers had, and it is fixed the
# same way: the directory goes in DIRS and gets a GNUmakefile whose
# install copies what is in it.
#
# CROSS ONLY, and there is no makefile beside this one.  Every file
# here is a SEED - passwd, group, motd, banner, ttys, signon and rc -
# and a native install would write them over a running system's own.
# /etc/passwd is the one that makes the point: installing the seed
# over the live file would throw away every account on the machine.
# The simulated filesystem is built from these once and then owns its
# copies, which is why only the cross build writes them.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

ROOT	?= ~/src/micronix/filesystem
ETCDIR	 = $(ROOT)/etc

#
# Named outright rather than globbed.  A wildcard here would also
# install whatever scratch file a later session leaves lying about,
# and the list is what tells a reader what /etc is supposed to hold.
#
FILES	= banner group motd passwd rc signon ttys

.PHONY: all update install install-man clean clobber

all:
	@echo "etc: $(words $(FILES)) files, nothing to build"

#
# update and install-man exist to be the same shape as every other
# directory, and do nothing.  The top level recurses over DIRS with
# them, so a directory that does not define one stops `make update`
# dead - which is what happened the first time etc was added here.
# There is nothing to ship: no native build reads these, so there is
# no etc.tar for the native update to unpack, and no man pages.
#
update install-man:
	@echo "etc: cross-installed only, nothing to ship"

install:
	mkdir -p $(ETCDIR)
#
# cp -p, and the -p is the whole of it.  Plain cp creates a file under
# the umask - 0600 on this machine, which would leave /etc/rc and the
# rest unreadable to anyone but their owner - and writing through an
# existing file keeps whatever mode it already had, so the same
# directory came out with the two modes mixed.  -p brings the source's
# mode with the contents, and the sources are 0644.
#
	cp -p $(FILES) $(ETCDIR)
	@echo "installed $(words $(FILES)) files in /etc"

clean clobber:
	@echo "etc: nothing to clean"
