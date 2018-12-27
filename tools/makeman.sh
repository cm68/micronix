#!/bin/bash
#
# bash script to generate man pages from source code in filesystem tree
#
mandir=../filesystem/usr/man
for dir in $mandir/man[0-9] ; do
	section=$(echo $dir | sed 's,man\([0-9]\),\1,' | sed "s,$mandir/,,")
	mkdir -p cat$section
	for file in $dir/*.$section ; do
		page=$(echo $file | sed "s,$mandir/man$section/,," | sed "s,\.$section,,")
		dest=$(echo $file | sed "s,$mandir/man$section/,,")
		../usersim/sim -d ../filesystem /bin/man $section $page > $mandir/cat$section/$dest
	done
done
