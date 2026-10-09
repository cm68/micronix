# install one pass of the distribution: mount /usr/src on drive 1, then
# copy each of four diskettes (fla..fld) onto the root with cptree.
# build_2drive writes this as /.login for the first pass; the same
# script drives the second pass, with disks 5-8 mounted as fla..fld.
mount /dev/hd1a /usr/src
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
mount /dev/flb /f -r ; cptree -v /f / ; umount /dev/flb
mount /dev/flc /f -r ; cptree -v /f / ; umount /dev/flc
mount /dev/fld /f -r ; cptree -v /f / ; umount /dev/fld
sync
echo === PASS-DONE
