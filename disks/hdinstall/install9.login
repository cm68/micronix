# the last install pass: a single diskette (fla), disk 9.  Written as
# /.login the way install5.login is, but only one cptree.
mount /dev/hd1a /usr/src
mount /dev/fla /f -r ; cptree -v /f / ; umount /dev/fla
sync
echo === PASS-DONE
