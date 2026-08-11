#
# libu - the micronix system call library
#
# lib/libu/GNUmakefile
#
# THE CROSS BUILD.  The compiler, its flags and its include path all
# come from GNUmakefile.inc at the top of the tree - this file says
# only what is particular to libu.  The makefile beside this one does
# the same thing natively inside the simulator, and the two are
# compared: the simulator is never truth, it is the check that the
# compiler generates equivalent code wherever it runs.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

DEPTH	= ../..
DIR	= libu
CLASS	= lib
TARGET	= libu.a

ASSRCS = access.s alarm.s _break.s chdir.s chmod.s chown.s close.s \
	creat.s dup.s errno.s exec.s execv.s exit.s fdpos.s fork.s fstat.s \
	getpid.s getuid.s gtty.s kill.s link.s mknod.s mount.s nice.s open.s \
	pause.s perror.s pipe.s read.s seekraw.s setuid.s _signal.s sleep.s \
	stat.s stime.s sbrk.s stty.s sync.s time.s umount.s unlink.s wait.s \
	write.s

CSRCS = signal.c lseek.c seek.c

include $(DEPTH)/GNUmakefile.inc

# What the shell needed and could not have: prove the symbols that
# signal() is built from are actually exported.  _signal.s had no
# .global declarations at all, so it assembled to an object that
# defined nothing and signal() would not link.
check: $(TARGET)
	@$(NM) $(TARGET) | \
		grep -E 'def +(__signal|_jtab|_stab|_signal)$$' \
		|| (echo "signal is still not exported" ; exit 1)
