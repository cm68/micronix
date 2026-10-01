#
# cross build for cp
#
# cmd/cp/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into cp.
#
# cptree is this program under a second name - the distribution's
# recursive tree copy, which the install scripts run as "cptree / /b -v".
# main() reads argv[0], sees cptree, and forces -r, so the one binary is
# both a plain cp and an always-recursive cptree.  See cp.c.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= cp
CLASS	= cmd

ALIAS	= cptree

include $(DEPTH)/GNUmakefile.inc
