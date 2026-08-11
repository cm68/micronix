#
# cross build for the commands
#
# cmd/GNUmakefile
#
# whitesmith is not in DIRS.  Its sources and its own makefiles are
# still in the tree, but nothing here builds them.
#
# These have a GNUmakefile but are not in DIRS, and so are not built:
#	ash asz cpmtools far init msh nroff yacc yacc.d
# That was true before this file was rewritten and is left alone; add
# them when they have been made to compile with ccc.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DIRS = ar cc cp echo form ld ls Make mkdir mknod pp.xx pwd rm s sh vi

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
