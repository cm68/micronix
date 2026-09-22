# makedebug.awk - write debug.h and dbgtags.c from the VERBOSE() tags
# fed to it on standard input, one per line, already sorted and unique.
#
# micronix/libexec/pass0/makedebug.awk
#
# Each tag is worth the next power of two, so the bits in a debug mask
# stay orthogonal no matter how many tags are added.  The two files are
# written here rather than in makedebug.sh because awk can reach two
# output streams at once and the shell's quoting cannot.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

BEGIN {
	print "/* created by makedebug.sh */" > "debug.h"
	print "/* created by makedebug.sh */" > "dbgtags.c"
	print "char *vopts[] = {" > "dbgtags.c"
	k = 1
}
{
	printf "#define %s 0x%02x\n", $1, k > "debug.h"
	printf "\"%s\",\n", $1 > "dbgtags.c"
	k = k * 2
}
END {
	print "0 };" > "dbgtags.c"
}
