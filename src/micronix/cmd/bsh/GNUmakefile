#
# cross build for bsh
#
# cmd/bsh/GNUmakefile
#
# The Bourne shell, the Seventh Edition sh: quoting, parameter and
# command substitution, redirection and here documents, the if, for,
# while, until, case and group compound commands, traps, and the
# builtins listed in msg.c.  There are no shell functions and no
# times builtin; the kernel has no times call to answer one.
#
# The compat directory the first port carried is gone: the tree's own
# <types.h>, <sys/dir.h> and <sys/stat.h> are what it compiles
# against, and the native makefile beside this one uses them too.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= bsh
CLASS	= cmd

CSRCS	= args.c blok.c builtin.c cmd.c ctype.c error.c expand.c fault.c \
	  globals.c io.c macro.c main.c msg.c name.c print.c service.c \
	  setbrk.c shim.c stak.c string.c word.c xec.c

HSRCS	= brkincr.h ctype.h defs.h dup.h mac.h mode.h name.h stak.h \
	  sym.h timeout.h

MANSRCS	= bsh.1

include $(DEPTH)/GNUmakefile.inc
