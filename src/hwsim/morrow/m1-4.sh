#!/bin/bash -f
x=
trace=
while getopts "xht:c:" opt ; do
	case "$opt" in
	x) x=-x
	;;
	h) help=-h
	;;
	c) conf="$OPTARG"
	;;
	t) trace="$OPTARG"
	;;
	esac
done
shift "$((OPTIND -1))"
if ! [ -z $trace ] ; then
	trarg="-t $trace"
fi
if ! [ -z $conf ] ; then
	confarg="-c $conf"
fi

export TERMINAL_FONTSIZE=14
./d1 -l $help $x -S micronix.sym $confarg $trarg ../disks/1010-8_stand-alone.IMD
stty cooked echo echoe
