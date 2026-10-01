#
# cross build for update
#
# cmd/update/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into update.
#
# EXCEPT THE PROGRAM'S OWN NAME.  GNUmakefile.inc carries an "update"
# target of its own - the one that unpacks a directory's sources into
# the simulated filesystem - and GNU make keeps the last rule for a
# name.  With TARGET defaulting to update there were two rules called
# update, the phony one came second and won, and the program was never
# linked.  So the target is named update.bin and INSTNAME says what it
# is installed as.  DIR is not touched: the sources still ship to
# /usr/src/cmd/update and the tar is still update.tar, and the phony
# update target still works.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= update
CLASS	= cmd
TARGET	= update.bin
INSTNAME = update

include $(DEPTH)/GNUmakefile.inc
