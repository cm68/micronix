#!/bin/bash -f
#
# wrapper script to try different kernels
#

root=../../..

x=""
tflag=""
conf=""
kernel=""

if [ -f .last ] ; then
	source .last
fi
rm -f .last

while getopts "ixk:t:c:" opt ; do
	case "$opt" in
	i) rm -f .last
	;;
	k) kernel="$OPTARG"
	;;
	x) x=-x
	;;
	c) conf="$OPTARG"
	;;
	t) tflag="$OPTARG"
	;;
	esac
done
shift "$((OPTIND -1))"

if ! [ -f $root/kernels/$kernel ] ; then
	echo $kernel not found in $root/kernels
	exit 1
fi

if ! [ -z "$tflag" ] ; then
	trarg="-t $tflag"
	echo "tflag=$tflag" >> .last
fi
if ! [ -z "$conf" ] ; then
	confarg="-c $conf"
	echo "conf=$conf" >> .last
fi
if ! [ -z "$x" ] ; then
	echo "x=-x" >> .last
fi

if [ -z "$kernel" ] ; then
	kernel=micronix.14
fi
echo "kernel=$kernel" >> .last

symfile=$root/kernels/$kernel.sym

disk=$root/disks/boot.IMD
cp $disk ./boot.IMD
rm ./boot.IMD-delta

$root/src/tools/mnix -f ./boot.IMD write $root/kernels/$kernel /micronix

export TERMINAL_FONTSIZE=14

rm -f core

echo "file ./d1" >.gdbargs
echo "set args -l $x -S $symfile $confarg $trarg ./boot.IMD" >> .gdbargs
./d1 -l $x -S $symfile $confarg $trarg ./boot.IMD
stty cooked echo echoe
