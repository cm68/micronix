#!/bin/sh
#
# Reproduce the IDE-leg /tmp directory-size corruption.
#
#	repro-dlook.sh <golden-dir> [runs]
#
# <golden-dir> holds a freshly installed root.img and usr.img (from an
# install-only matrix leg).  Each run restores both from that snapshot,
# writes a probe /.login that mounts /usr/src and compiles cmd/ld/ld.c,
# boots the IDE disk, and then reads /tmp's inode size back on the host.
#
# A clean /tmp is 32 bytes (". and .."); the failure is a high word
# merged into the size, e.g. 16515136 = 0xFC0040, after which every
# create in /tmp fails EFBIG.
#

repo=/home/curt/micronix
golden=${1:?usage: $0 <golden-dir> [runs]}
runs=${2:-3}

root="$repo/src/hwsim/d1"
mnix="$repo/src/tools/mnix"

[ -f "$golden/root.img" ] || { echo "$0: $golden/root.img missing" >&2; exit 1; }
[ -f "$golden/usr.img" ] || { echo "$0: $golden/usr.img missing" >&2; exit 1; }

r=1
while [ "$r" -le "$runs" ]; do
	w=/tmp/repro-dlook/$r
	rm -rf "$w"
	mkdir -p "$w/drives" || exit 1
	cp "$golden/root.img" "$w/root.img" || exit 1
	cp "$golden/usr.img" "$w/usr.img" || exit 1

	cat > "$w/probe.login" <<'EOF'
mount /dev/ide1 /usr/src
cd /usr/src/cmd/ld
ccc -O -c ld.c
echo === CCC-DONE
sync
halt
EOF

	"$mnix" -f "$w/root.img" write "$w/probe.login" /.login || exit 1

	( cd "$root" && timeout 300 setsid script -qec "./d1p -d $w/drives -w multio,vi6=hddma,vi6=hdca,vi6=disk2,vi6=disk3,vi7=disk1 -L $w/log ide0:$w/root.img ide1:$w/usr.img" /dev/null </dev/null >/dev/null 2>&1 )

	size=`"$mnix" -f "$w/root.img" ls / 2>/dev/null | awk '$NF == "tmp" { print $(NF-1) }'`
	errs=`grep -a -c "cannot create\|cannot replace\|File too large" "$w/log" 2>/dev/null`

	verdict=clean
	if [ -n "$size" ] && [ "$size" -gt 1000 ] 2>/dev/null; then
		verdict=CORRUPT
	fi
	echo "run $r: tmp size = ${size:-?} (${verdict}), error lines = ${errs:-0}, log $w/log"

	r=`expr $r + 1`
done
