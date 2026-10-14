# Install the 8 inch distribution from the media changer, one diskette
# at a time.  build_dist writes this as /.login; the single-user shell
# runs it, so a boot needs nothing typed.  Each diskette is loaded onto
# drive 0 with chg, mounted on /f, and cptree'd onto the root; /usr/src
# is on drive 1 and mounted first so the sources land there.  The umount
# between disks flushes the drive's blocks (bflush, uio.c), so the next
# disk reads fresh.
mount /dev/hd1a /usr/src

chg load dist8:0 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:1 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:2 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:3 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:4 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:5 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:6 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:7 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
chg load dist8:8 onto djdma:0
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla

sync
echo === PASS-DONE
