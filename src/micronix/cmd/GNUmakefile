#
# cross build for the commands
#
# cmd/GNUmakefile
#
# WHAT THIS DIRECTORY IS FOR: the source of everything that goes into
# /bin.  One directory per program, and if a program is on the system
# its source is here.
#
# What does not go into /bin is in ../attic - dead compilers, the
# assembler that came before asz, two makes that lost the name, and
# the shells that were read before this one.  Nothing there is built
# and nothing here refers to it.
#
# Still here and still not in DIRS, because they are programs that DO
# belong on the system and are not ready:
#
#   far cpmtools mkfs mt
#
# awk is here too and not in DIRS: the 2.11BSD port compiles and links
# but is ~72K, too large for micronix's flat 64K address space, so it
# has to be slimmed down first.  cmd/awk/README.md has the size analysis.
#
# yacc is here too and builds - cmd/yacc - but is not in DIRS yet: the
# two-pass table sizing is in, but nothing has compiled the parser it
# generates, so liby.a and the templates have only been smoke-tested.
#
# And the compiler, moving into this tree one directory per pass and
# per tool:
#
#	cpp c0 c1 peep		the passes
#	asz ld nm ar size ccc astpp	the tools
#
# Those were prototypes.  Their sources are this tree's own now,
# forked from ~/src/ccc,
# which is still where they are edited.  Most are not in DIRS: they are
# host programs today, and building them here means building the
# compiler for the Z80, which is where this is going but is not there
# yet.  Add them when they have been made to compile with ccc.
#
# ld, nm and size are in DIRS now - they took their real names from
# wsld, wsnm and wssize, and they are ordinary commands rather than
# passes.
#
# init builds.  It is a reconstruction from the disassembly of the 1.6
# binary, and the helpers that binary carries are now written into it -
# six of them, not the five listed here before: putstr was undefined
# too and is at 0x3422.  They are library routines rather than init's
# own, sitting among malloc, atoi and strlen in the binary, so they are
# at the end of init.c under a heading that says so and gives their
# addresses.  When libc grows them they come out again.
#
# It has never been run.  Linking is not the same as working, and this
# is process 1: the thing to try it on is a boot, not a shell.
#
# man is a reconstruction too, from /bin/man, and is built here for the
# same reason.  It is also help - one binary under two names, which main()
# tells apart by looking for "help" inside argv[0] - so the native install
# links /bin/help to it rather than building a second program.  Its two
# man pages were already in the tree and agree with what the disassembly
# says; cmd/man/README has the details, including the two bugs in the
# original that man.c reproduces.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

#
# The 2.11BSD ports - cat chgrp chmod cmp date dd diff du grep ln mv
# od pr sed tee - came in through a NEW list that nothing invoked,
# and moved up here once they had built, tested against their host
# counterparts, and shed their compiler workarounds.
#
DIRS = ar asz cat ccc chgrp chmod cmp cp date dcheck dd diff du echo ed fgrep find form grep icheck init ld less ln ls make man mkdir mknod more mv ncheck nm od pr pwd rm s sed setdev sh size strings sync tar tee upm vi wc

#
# "cd $$d &&", not "cd $$d ;".  With the semicolon, a directory named
# here that does not exist leaves the subshell in cmd/, where it runs
# the same rule over again - which is what DIRS saying "pp" when the
# directory is "pp.xx" did, forty levels deep until make gave up.
#
RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

#
# This directory has a native makefile of its own - the one that runs
# "make install" in each command in turn - and the recursion alone
# would leave /usr/src/cmd holding the subdirectories and nothing to
# drive them.  So update ships it, the same way a command's own
# GNUmakefile ships that command's, and --keep-newer-files for the
# same reason: a makefile edited inside micronix is not to be
# overwritten by one older than it.
#
ROOT	?= ~/src/micronix/filesystem
DEST	 = $(ROOT)/usr/src/cmd
NOWARN	 = --warning=no-ignore-newer

.PHONY: all update install install-man clean clobber doc

all:
	$(RECURSE)

#
# reset rides with it.  It is a shell script and not a program, so it
# has no directory of its own and no makefile to ship it; the native
# makefile beside this one installs it with "cp/cp reset /bin/reset"
# and had nothing to copy, because update shipped the makefile that
# names it and not the file itself.  Every native build ended with
# "cp: reset : No such file or directory" and no reset on the system.
#
# The same shape as crt0.s in lib/libu: a file no rule mentions is not
# missed until something asks for it.
#
cmd.tar: makefile reset
	tar -f cmd.tar -c makefile reset

update: cmd.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f cmd.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	$(RECURSE)

#
# Nothing of its own here; this exists so that the rule reaches the
# directories below, which do have pages.
#
install-man:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f cmd.tar

doc:
	$(RECURSE)
