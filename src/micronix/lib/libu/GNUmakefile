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
	creat.s dup.s errno.s exec.s execv.s exit.s _exit.s fdpos.s fork.s fstat.s \
	getpid.s getuid.s gtty.s kill.s link.s mknod.s mount.s nice.s open.s \
	pause.s perror.s pipe.s read.s seekraw.s setuid.s _signal.s sleep.s \
	stat.s stime.s sbrk.s stty.s sync.s time.s umount.s unlink.s wait.s \
	write.s fputc.s fgetc.s

#
# rename.c is here rather than in libc because the API is what is
# being held still, not what is under it.  Micronix is a v6 and has
# no rename call - it arrived in 4.2BSD - so today this is a link and
# an unlink written in C.  The day the kernel grows the call, this
# becomes an .s stub shaped like link.s and nothing above it changes.
# It is a system call as far as anything calling it is concerned, so
# it lives with the system calls.
#
CSRCS = signal.c lseek.c seek.c rename.c \
	buf.c cleanup.c fclose.c fflush.c filbuf.c flsbuf.c \
	fopen.c fread.c freopen.c fwrite.c fseek.c ftell.c \
	rewind.c setbuf.c ungetc.c gets.c puts.c putchar.c \
	printf.c fprintf.c scanf.c fscanf.c isatty.c

#
# crt0 rides beside the archive and not inside it: it is the startup
# the linker puts in front of a program, named by the driver, so it
# must not be one of the members ar packs into libu.a.  That is why
# it is not in ASSRCS - OBJS is built from that list and OBJS is what
# is archived - and it is named here instead, so that SRCS ships it
# and a rule of its own builds and installs it.
#
# It had neither before.  crt0.s was in no source list, so update
# never copied it into the simulator's tree; crt0.o was in no object
# list, so nothing built it and nothing installed it.  The copy in
# /lib was put there by hand when the system was bootstrapped and
# then owned by no makefile at all, which is how it was still calling
# main with argc and argv both on the stack long after the first
# argument had moved into HL.  Every native build linked against it
# and produced programs that read their own arguments as garbage.
#
CRT0	= crt0.o
SRCS	= $(CSRCS) $(HSRCS) $(ASSRCS) $(MANSRCS) crt0.s

include $(DEPTH)/GNUmakefile.inc

all: $(CRT0)

$(CRT0): crt0.s
	$(CCC) $(CCFLAGS) -c crt0.s

install: installcrt0

installcrt0: $(CRT0)
	mkdir -p $(LIBINSTDIR)
	cp $(CRT0) $(LIBINSTDIR)/$(CRT0)

clean: cleancrt0

cleancrt0:
	rm -f $(CRT0)

# What the shell needed and could not have: prove the symbols that
# signal() is built from are actually exported.  _signal.s had no
# .global declarations at all, so it assembled to an object that
# defined nothing and signal() would not link.
check: $(TARGET)
	@$(NM) $(TARGET) | \
		grep -E 'def +(__signal|_jtab|_stab|_signal)$$' \
		|| (echo "signal is still not exported" ; exit 1)
