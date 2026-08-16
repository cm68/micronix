#!/bin/bash
echo "/* created by makedebug.sh */" > debug.h
echo "/* created by makedebug.sh */" > dbgtags.c
echo "char *vopts[] = {" >> dbgtags.c
k=1
tags=($(grep VERBOSE *.c | sed -e 's/^.*VERBOSE(//' -e 's/).*$//' | sort -u))
for tag in ${tags[@]}; do
	printf "#define $tag 0x%02x\n" $k >> debug.h
	printf "\"$tag\",\n" >> dbgtags.c
	(( k = k << 1 ))
done
echo "0 };" >> dbgtags.c
