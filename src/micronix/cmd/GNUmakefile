#
# cross build for the commands
#
# cmd/GNUmakefile
#
# whitesmith is not in DIRS.  Its sources and its own makefiles are
# still in the tree, but nothing here builds them.
#
# ld is not in DIRS either, and for the same reason hitech left the
# build: it is a link editor for the hitech object format, which
# nothing here produces any more.  wsld is the linker this toolchain
# uses, and it is to be renamed ld and take this one's place.  The
# source stays until then.
#
# pp.xx is out for the same kind of reason - a post-processor for a
# compiler this tree stopped using, which has not tracked anything in
# a long time.  Kept, not built.
#
# cc is out too: it is the v7 cc driver swizzled to run hitech's and
# whitesmith's passes - cp1, cp2, optim, link, with -H to pick hitech -
# and both of those compilers have left this build.  ccc is the driver
# now.  Kept, not built.
#
# These have a GNUmakefile but are not in DIRS, and so are not built:
#	ash asz cpmtools far msh nroff yacc yacc.d
#
# make is the make micronix uses - it was cmd/Make until the v7 one
# gave up the name, and that one is cmd/v7make now.  make.d is a third
# make and is also not built.
#
# The compiler is moving into this tree, one directory per pass and
# per tool:
#
#	cpp c0 c1 peep		the passes
#	asz wsld wslib wsnm wssize ccc astpp	the tools
#
# Those are prototypes.  Their sources are symlinks into ~/src/ccc,
# which is still where they are edited, and they are not in DIRS: they
# are host programs today, and building them here means building the
# compiler for the Z80, which is where this is going but is not there
# yet.  asz among them replaces the TRASM-derived assembler, which is
# cmd/asz.old.
# That was true before this file was rewritten and is left alone; add
# them when they have been made to compile with ccc.
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
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = ar cp echo form init ls make mkdir mknod pwd rm s sh vi

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

.PHONY: all update install clean clobber doc

all:
	$(RECURSE)

cmd.tar: makefile
	tar -f cmd.tar -c makefile

update: cmd.tar
	$(RECURSE)
	mkdir -p $(DEST)
	tar -f cmd.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)
	rm -f cmd.tar

doc:
	$(RECURSE)
