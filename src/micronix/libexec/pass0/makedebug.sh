#!/bin/sh
#
# makedebug.sh - regenerate debug.h and dbgtags.c from the VERBOSE()
# calls in the sources
#
# micronix/libexec/pass0/makedebug.sh
#
# Runs natively under /bin/sh, so it sticks to what that shell is: no
# variables beyond $0..$9, no loops, no arrays, and single quotes are
# not special - which is why the sed script is quoted with double
# quotes and the awk program lives in makedebug.awk beside this file.
# The pipeline is one line because the shell reads a line at a time.
# sed picks the tags out, sort orders them (the native sort has no
# -u), and the awk drops the duplicates and writes the two files.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

sed -n -e "s/^.*VERBOSE(\([^)]*\)).*$/\1/p" *.c | sort | awk -f makedebug.awk
