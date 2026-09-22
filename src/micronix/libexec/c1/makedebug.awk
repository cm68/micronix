# makedebug.awk - write debug.h and dbgtags.c from the VERBOSE() tags
# on stdin, one per line, already sorted.  Duplicates are dropped here
# because the native sort has no -u, and each tag takes the next power
# of two so the bits stay orthogonal as tags are added.  Both files are
# written here rather than in makedebug.sh because awk can reach two
# output streams at once and the shell's quoting cannot.
#
# micronix/libexec/c1/makedebug.awk

BEGIN {
	print "/* created by makedebug.sh */" > "debug.h"
	print "/* created by makedebug.sh */" > "dbgtags.c"
	print "char *vopts[] = {" > "dbgtags.c"
	k = 1
}
{
	if ($1 == tag)
		next
	tag = $1
	printf "#define %s 0x%02x\n", $1, k > "debug.h"
	printf "\"%s\",\n", $1 > "dbgtags.c"
	k = k * 2
}
END {
	print "0 };" > "dbgtags.c"
}
