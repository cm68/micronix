#!/bin/bash
WSPATH=~/micronix/whitesmith/C/V2-0
MINCE=/Users/cmayer/archives/rlee/cpmarchives/cpm/Software/rlee/U/UNICORN/MINCE
FORMAT=z80pack-hd
IMAGE=ws.dsk

rm -f $IMAGE
dd if=/dev/zero of=$IMAGE bs=1024 count=4080
mkfs.cpm -f $FORMAT $IMAGE
cpmcp -f $FORMAT $IMAGE $WSPATH/* 0:
cpmcp -f $FORMAT $IMAGE $MINCE/*.COM 0:
cpmcp -f $FORMAT $IMAGE $MINCE/mince.swp.xterm 0:mince.swp

rm -f disks/drivei.dsk
ln -s $IMAGE disks/drivei.dsk
