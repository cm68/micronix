#!/bin/bash
notrim=false
if [ $1 == '-n' ] ; then
	notrim=true
	shift
fi

function setdrive
{
	if (( $# != 4 )) ; then
		echo "usage: <file> 8|5 skew|noskew 512|1024"
		exit
	fi

	orig=$1
	if [ ! -f $orig ] ; then
		echo $orig does not exist
		exit
	fi

	echo $1 $2 $3 $4

	case $2 in
	8)
		type=0
		;;
	5)
		type=4
		;;
	*)
		echo unknown drive type
		exit
		;;
	esac

	case $3 in
	noskew)
		alt=0
		;;
	skew)
		alt=8
		;;
	*)
		echo unknown skew type
		exit
		;;
	esac

	k=
	case $4 in
	512)
		size=0
		;;
	1024)
		k=.1k
		size=16
		;;
	*)
		echo unknown skew type
		exit
		;;
	esac

	new=$(basename $orig .bin).$2.$3$k

	echo skew: $alt type: $type size: $size

	drive=0
	carry=0
	while true ; do
		((minor=$carry+$drive+$type+$alt+$size))
		echo $minor
		link="bdev(2,$minor)"
		echo checking $link
		if [ -L $link ] ; then
			echo in use
			if [ $drive == 3 ] ; then
				((carry+=32))
				drive=0
			else
				((drive++))
			fi
			continue;
		fi
		break	
	done
	ln -s $trim $link
	ln -s $link $new
}

function work
{
	orig=$1
	if $notrim ; then
		trim=$orig
	else
		trim=$(basename $orig .bin).trim20k
		dd if=$orig of=$trim bs=1 skip=20480
	fi

	for g in 8 5 ; do
		for s in noskew skew ; do
			for w in 512 1024 ; do
				setdrive $1 $g $s $w
			done
		done
	done
}

for file in $* ; do
	work $file
done
