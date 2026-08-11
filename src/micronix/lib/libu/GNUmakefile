#
# libu - the micronix system call library
#
# lib/libu/GNUmakefile
#
# THE CROSS BUILD.  Uses the installed compiler in /usr/local/bin,
# builds in the tree, and is the one that produces truth.  The
# makefile beside this does the same thing natively inside the
# simulator, and the two are compared - the simulator is never truth,
# it is the check that the compiler generates equivalent code
# wherever it runs.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

CCC	= /usr/local/bin/ccc
CCFLAGS	= -m micronix

# The archiver is NOT installed.  ccc's "make install" puts the
# compiler, the passes and the libraries into /usr/local but leaves
# wslib and wsnm behind, so nothing installed can build or inspect an
# archive.  Until that is fixed this reaches into the compiler tree,
# which is exactly the sort of thing this policy exists to stop.
LIB	= /vault/src/ccc/unix/bin/wslib

ARCHIVE	= libu.a

SRCS = access.s alarm.s _break.s chdir.s chmod.s chown.s close.s \
	creat.s dup.s errno.s exec.s execv.s exit.s fdpos.s fork.s fstat.s \
	getpid.s getuid.s gtty.s kill.s link.s mknod.s mount.s nice.s open.s \
	pause.s perror.s pipe.s read.s seekraw.s setuid.s _signal.s sleep.s \
	stat.s stime.s sbrk.s stty.s sync.s time.s umount.s unlink.s wait.s \
	write.s

CSRCS = signal.c lseek.c seek.c

OBJS = $(SRCS:.s=.o) $(CSRCS:.c=.o)

all: $(ARCHIVE)

%.o: %.s
	$(CCC) $(CCFLAGS) -c $<

%.o: %.c
	$(CCC) $(CCFLAGS) -I/usr/local/lib/include -c $<

$(ARCHIVE): $(OBJS)
	rm -f $@
	$(LIB) -cr $@ $(OBJS)

# What the shell needed and could not have: prove the symbols that
# signal() is built from are actually exported.  _signal.s had no
# .global declarations at all, so it assembled to an object that
# defined nothing and signal() would not link.
check: $(ARCHIVE)
	@/vault/src/ccc/unix/bin/wsnm $(ARCHIVE) | \
		grep -E 'def +(__signal|_jtab|_stab|_signal)$$' \
		|| (echo "signal is still not exported" ; exit 1)

# Refresh the simulated filesystem so the native build can be run
# against the same source.  Narrow purpose: verification, not truth.
DEST = $(HOME)/src/micronix/filesystem/usr/src/lib/libu
update:
	mkdir -p $(DEST)
	cp makefile $(SRCS) $(CSRCS) $(DEST)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(ARCHIVE)
