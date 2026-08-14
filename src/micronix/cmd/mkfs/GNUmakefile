#
# cross build for mkfs
#
# cmd/mkfs/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into mkfs.
#
# Not in cmd's DIRS: this makes a file system, and what it writes has
# to match what the kernel reads, which wants checking before it is
# built by default.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= mkfs
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc
