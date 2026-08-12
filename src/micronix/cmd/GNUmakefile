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
#	ash asz cpmtools far init msh nroff yacc yacc.d
# That was true before this file was rewritten and is left alone; add
# them when they have been made to compile with ccc.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = ar cp echo form ls Make mkdir mknod pwd rm s sh vi

#
# "cd $$d &&", not "cd $$d ;".  With the semicolon, a directory named
# here that does not exist leaves the subshell in cmd/, where it runs
# the same rule over again - which is what DIRS saying "pp" when the
# directory is "pp.xx" did, forty levels deep until make gave up.
#
RECURSE = @for d in $(DIRS) ; do \
		( cd $$d && $(MAKE) $@ ) || exit 1 ; \
	done

.PHONY: all update clean clobber doc

all:
	$(RECURSE)

update:
	$(RECURSE)

clean:
	$(RECURSE)

clobber:
	$(RECURSE)

doc:
	$(RECURSE)
