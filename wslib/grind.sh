#!/bin/bash
#
# disassemble the whitesmith's library
#
# wslib/grind.sh 
# Changed: <2021-12-23 15:25:40 curt>
#
for i in ../filesystem/lib/*.a ; do
	d=$(basename $i .a)
	mkdir $d
	(cd $d ; ../../src/tools/ar -x ../$i )
done

mkdir crt
cp ../filesystem/lib/*.o crt

for i in $(find . -type d -print) ; do
	echo $i
	for f in $(find $i -type f -name \*.o -print) ; do
		../src/tools/disas $f > $(dirname $f)/$(basename $f .o).s
	done
done
