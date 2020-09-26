#!/bin/bash -f
x=""
tflag=""
while getopts "xt:c:" opt ; do
	case "$opt" in
	x) x=-x
	;;
	c) conf="$OPTARG"
	;;
	t) tflag="$OPTARG"
	;;
	esac
done
shift "$((OPTIND -1))"
if ! [ -z $tflag ] ; then
	trarg="-t $tflag"
fi
if ! [ -z $conf ] ; then
	confarg="-c $conf"
fi
disk=../disks/1010-8_stand-alone.IMD

export TERMINAL_FONTSIZE=14
echo "file ./d1" >.gdbargs
echo "set args -l $x -S micronix.sym $confarg $trarg $disk" >> .gdbargs
./d1 -l $x -S micronix.sym $confarg $trarg $disk
stty cooked echo echoe
