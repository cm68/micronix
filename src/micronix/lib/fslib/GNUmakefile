#
# libfs - the filesystem access library
#
# lib/fslib/GNUmakefile
#
# Two halves: fslibfunc.c is the portable inode/block/directory logic,
# shared with the host fslib; fslib.c is the driver, the block device
# and the superblock.  The host build links fslibfunc.c against its own
# .IMD/drive-file driver; this one links both against the kernel's
# block devices.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= fslib
CLASS	= lib
TARGET	= libfs.a

include $(DEPTH)/GNUmakefile.inc
