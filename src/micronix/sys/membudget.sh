#!/bin/sh
#
# membudget - the kernel's memory budget, measured from the built image.
#
# Read-only.  The numbers come out of the link itself: sys/unix's object
# header and symbol table, the folded objects the final link places
# (low.o, highmem.o, ovlslot.o, win.o, upage.o) and the driver modules.
# Nothing is rebuilt, so build sys/unix first.
#
# sizeof(struct buf) is the one figure no header carries.  It is what a
# pool header costs, so it is compiled out of the tree's own headers
# with mxccc; textpad.s's 8-header reservation is the cross-check, and a
# disagreement is reported, because it means the pool does not start
# where _ebss says it does.
#
# The layout (include/sys/sys.h, sys/GNUmakefile):
#
#   resident text/data/bss  0x1000 up to _ebss
#   highmem.o               parked at _ebss, may run to OVLBASE
#   buffer headers          _ebss (blist+8) up to SCRWIN
#   driver slot             OVLBASE   (OVLSEG, one 4K page)
#   buffer window           BUFWIN    (BUFSEG)
#   u page                  USERSEG, one 4K page
#
# Usage: membudget.sh [image]        (default: unix beside this script)

set -e

here=$(cd "$(dirname "$0")" && pwd)
top=$(cd "$here/../../.." && pwd)
bin=$top/bin
inc=$here/../include
sys=$here
image=${1:-$sys/unix}

mxnm=$bin/mxnm
mxccc=$bin/mxccc
PAGE=4096

[ -f "$image" ] || { echo "membudget: $image not found - build it first" >&2; exit 1; }
[ -x "$mxnm" ] || { echo "membudget: $mxnm not found" >&2; exit 1; }

die() { echo "membudget: $*" >&2; exit 1; }

# text data bss heap textoff dataoff, out of a Whitesmiths object header
hdr() { od -An -tu2 -j4 -N12 "$1"; }

# a symbol's address, in hex without 0x.  mxnm prints
#
#	  0xafb9  bss      global  def    _ebss
#
# and the older src/tools/nm prints "name: addr"; take either.
sym() {
	"$mxnm" "$image" | awk -v want="$1" '
		$2 == want ":" { print $3; exit }
		$NF == want    { a = $1; sub(/^0x/, "", a); print a; exit }'
}

# a segment number from include/sys/sys.h
segof() { awk -v want="$1" '$1 == "#define" && $2 == want { print $3; exit }' "$inc/sys/sys.h"; }

hex() { printf '0x%x' "$1"; }

scrwin=$(( $(segof SCRSEG) * PAGE ))
bufwin=$(( $(segof BUFSEG) * PAGE ))
useg=$(( $(segof USERSEG) * PAGE ))
ovlseg=$(awk '$1 == "OVLSEG" && $2 == "=" { print $3; exit }' "$sys/GNUmakefile")
[ -n "$ovlseg" ] || die "no OVLSEG in $sys/GNUmakefile"
ovlbase=$(( ovlseg * PAGE ))
maxbufs=$(awk '$1 == "#define" && $2 == "MAXBUFS" { if ($3 + 0 > m) m = $3 + 0 } END { print m }' "$sys/main.c")

ebss=$(sym _ebss)
blist=$(sym _blist)
[ -n "$ebss" ] || die "no _ebss in $image"
[ -n "$blist" ] || die "no _blist in $image"
ebss=$(( 0x$ebss ))
blist=$(( 0x$blist ))

# sizeof(struct buf): compile the one expression, and cross-check it
# against what textpad.s reserved for blist's 8 boot headers.
hsz=
tmp=$(mktemp -d "${TMPDIR:-/tmp}/membudget.XXXXXX") || die "mktemp failed"
trap 'rm -rf "$tmp"' 0 1 2 15
printf '#include <types.h>\n#include <sys/buf.h>\nchar x[sizeof(struct buf)];\n' > "$tmp/sz.c"
if [ -x "$mxccc" ] && ( cd "$tmp" && "$mxccc" -m micronix -O -i"$inc" -I"$inc" -c sz.c ) >/dev/null 2>&1; then
	hsz=$(od -An -tu2 -j8 -N2 "$tmp/sz.o" | tr -d ' \n')
fi
derived=$(( (ebss - blist) / 8 ))
[ -n "$hsz" ] || hsz=$derived
start=$(( blist + 8 * hsz ))

# resident: low.o is text, data and bss as the final link places them
set -- $(hdr "$sys/low.o")
lt=$1; ld=$2; lb=$3
set -- $(hdr "$image")
toff=$5; doff=$6
[ "$toff" -ne 0 ] || toff=0x1000
[ "$doff" -ne 0 ] || doff=$(( toff + lt ))
dataend=$(( doff + ld ))
bssend=$(( dataend + lb ))
extent=$(( ebss - toff ))
resfree=$(( scrwin - ebss ))

# init-only code: parked at _ebss, may reach OVLBASE
set -- $(hdr "$sys/highmem.o")
hsize=$(( $1 + $2 + $3 ))
hend=$(( ebss + hsize ))
linkfree=$(( ovlbase - ebss - hsize ))

# buffer pool: headers from blist+8*sizeof to SCRWIN
room=$(( scrwin - start ))
nnew=0
if [ "$room" -ge "$hsz" ]; then nnew=$(( room / hsz )); fi
nbuf=$(( 8 + nnew ))
if [ "$nbuf" -gt "$maxbufs" ]; then
	nbuf=$maxbufs
	nnew=$(( nbuf - 8 ))
fi
segments=$(( 1 + (nnew + 7) / 8 ))
spare=$(( room - nnew * hsz ))

printf 'kernel memory budget: %s\n' "$image"
printf '\n'
printf 'resident    text   %s-%s  %6d\n' "$(hex "$toff")" "$(hex $(( toff + lt )))" "$lt"
printf '            data   %s-%s  %6d\n' "$(hex "$doff")" "$(hex "$dataend")" "$ld"
printf '            bss    %s-%s  %6d\n' "$(hex "$dataend")" "$(hex "$ebss")" "$lb"
printf '            extent %s-%s  %6d  SCRWIN %s, %d free\n' \
	"$(hex "$toff")" "$(hex "$ebss")" "$extent" "$(hex "$scrwin")" "$resfree"
printf '\n'
printf 'init-only   highmem.o %s-%s  %6d  OVLBASE %s, %d free\n' \
	"$(hex "$ebss")" "$(hex "$hend")" "$hsize" "$(hex "$ovlbase")" "$linkfree"
printf '            link needs _ebss <= %s: %d bytes of resident growth\n' \
	"$(hex $(( ovlbase - hsize )))" "$linkfree"
printf '\n'
printf 'pool        %d bytes/header: 8 boot + %d = %d buffers (MAXBUFS %d)\n' \
	"$hsz" "$nnew" "$nbuf" "$maxbufs"
printf '            headers %s-%s, %d segments = %dK of buffer data, %d bytes spare\n' \
	"$(hex "$start")" "$(hex $(( start + nnew * hsz )))" "$segments" \
	"$(( segments * PAGE / 1024 ))" "$spare"
printf '            every %d bytes of resident growth costs one buffer\n' "$hsz"
printf '\n'
printf 'pages       driver slot %s  4096/4096\n' "$(hex "$ovlbase")"
set -- $(hdr "$sys/win.o")
printf '            window      %s  %4d/4096  %4d free\n' "$(hex "$6")" "$(( $2 + $3 ))" "$(( PAGE - $2 - $3 ))"
set -- $(hdr "$sys/upage.o")
printf '            u page      %s  %4d/4096  %4d free\n' "$(hex "$6")" "$(( $2 + $3 ))" "$(( PAGE - $2 - $3 ))"
printf '\n'
printf 'modules     one 4096-byte page at %s\n' "$(hex "$ovlbase")"
mods=$(awk '$1 == "MODS" && $2 == "=" { sub(/^MODS[ \t]*=[ \t]*/, ""); print; exit }' "$sys/GNUmakefile")
for m in $mods; do
	if [ -f "$sys/$m.mod" ]; then
		printf '%d %s %d\n' "$(( PAGE - $(wc -c < "$sys/$m.mod") + 16 ))" "$m" \
			"$(( $(wc -c < "$sys/$m.mod") - 16 ))"
	else
		printf '%d %s 0\n' "-1" "$m"
	fi
done | sort -n | while read -r free m use; do
	if [ "$free" -lt 0 ]; then
		printf '            %-8s not built\n' "$m"
	else
		printf '            %-8s %5d  %5d free\n' "$m" "$use" "$free"
	fi
done
built=0
for m in $mods; do
	if [ -f "$sys/$m.mod" ]; then built=$(( built + 1 )); fi
done
printf '            %d pages, %d bytes stacked after the link\n' \
	"$built" "$(( built * PAGE ))"

if [ "$hsz" != "$derived" ]; then
	printf '\nwarning: sizeof(struct buf) is %d but blist+8 headers span %d bytes;\n' \
		"$hsz" "$(( ebss - blist ))"
	printf '         the pool is counted from %s, not _ebss %s.\n' "$(hex "$start")" "$(hex "$ebss")"
fi
if [ "$bssend" -ne "$ebss" ]; then
	printf '\nwarning: low.o data+bss ends at %s, _ebss says %s.\n' \
		"$(hex "$bssend")" "$(hex "$ebss")"
fi
