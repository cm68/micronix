#
# cross build for sh
#
# cmd/sh/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into sh.
#
# The kill builtin is out of the build - /bin/kill is the command now,
# and a builtin shadows the path.  What is left of the old one is
# behind BUILTIN_KILL: the case in sh.c, jobpid in job.c, and the table
# entry.  Uncomment the line at the bottom to put it back.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= sh
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

# CCFLAGS += -DBUILTIN_KILL
