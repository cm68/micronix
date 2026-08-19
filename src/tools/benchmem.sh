#!/bin/bash
# benchmem.sh - measure each compiler pass's peak memory over the corpus.
#
# Runs the Z80 passes - pass0 (cpp), c0 (pass1), c1 (pass2) - under the
# usersim with -S process accounting, over every .c source under
# filesystem/usr/src, and records the gap per pass.  "gap" is the free
# memory between the break and the stack's low-water mark: the smaller
# the gap, the closer that pass came to running out of memory on that
# source.  The source with the smallest gap is the worst offender.
#
# Why -S: the passes' static size (mxsize) is not the same thing as
# their dynamic memory under load.  A pass can be small but buffer a
# huge input, or large but stream.  This measures what actually matters.
#
# Output (in $OUTDIR, default the repo root):
#   benchmem.tsv    one line per (pass, source): pass source bytes gap heap exit
#   benchmem.worst  the single worst source per pass (minimum gap)
#
# The corpus is processed largest source first, so the worst offenders
# come out early if the run is cut short.
#
# Usage:
#   src/tools/benchmem.sh            # full corpus
#   src/tools/benchmem.sh EXPR       # only sources whose path matches EXPR
#   OUTDIR=/tmp/bench src/tools/benchmem.sh
#
# To re-derive the summary from a finished benchmem.tsv without
# re-running the benchmark, use benchmem_summary.sh.

set -u

here=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$here/../.." && pwd)

SIM="$TOP/bin/sim"
ROOT="$TOP/filesystem"
PASS0=/libexec/pass0
C0=/libexec/c0
C1=/libexec/c1
WORK=/tmp/bench

# The include directories a source may need, as paths inside the sim's
# filesystem (the -d root).  The source's own directory is added first,
# then every place a header lives, then the system include dir.
INC_BASE="-I/usr/src/libexec/cpp -I/usr/src/libexec/c0 -I/usr/src/libexec/c1 \
-I/usr/src/lib/libccc -I/usr/src/lib/include -I/usr/src/include \
-i/usr/include"

TMO=${TMO:-240}                 # seconds before a pass is declared hung
OUTDIR=${OUTDIR:-"$TOP"}

FILTER=${1:-}

mkdir -p "$OUTDIR" "$ROOT$WORK"
OUT="$OUTDIR/benchmem.tsv"
WORST="$OUTDIR/benchmem.worst"

min0=999999; min1=999999; min2=999999
w0=""; w1=""; w2=""

# Run one pass, print "gap heap exit" (or "- - -" on failure/timeout).
acct() {
    local out gap heap ec
    out=$(timeout "$TMO" "$SIM" -S -d "$ROOT" "$@" 2>&1)
    gap=$(printf '%s\n' "$out" | grep -o 'gap=[0-9]*' | head -1 | cut -d= -f2)
    heap=$(printf '%s\n' "$out" | grep -o 'heap=[0-9]*' | head -1 | cut -d= -f2)
    ec=$(printf '%s\n' "$out" | grep -o 'exit=[0-9]*' | head -1 | cut -d= -f2)
    printf '%s %s %s' "${gap:--}" "${heap:--}" "${ec:--}"
}

while IFS= read -r src; do
    [ -n "$FILTER" ] && case "$src" in *"$FILTER"*) ;; *) continue;; esac
    dir=$(dirname "$src")
    sz=$(wc -c < "$ROOT$src" 2>/dev/null)

    read g0 h0 e0 <<< "$(acct "$PASS0" -DCCC -DMICRONIX -I"$dir" $INC_BASE -o "$WORK/b" "$src")"

    g1="-"; h1="-"; e1="-"
    [ "$e0" = "0" ] && read g1 h1 e1 <<< "$(acct "$C0" "$WORK/b.x" "$WORK/b.ast" "$WORK/b.dat")"

    g2="-"; h2="-"; e2="-"
    [ "$e1" = "0" ] && read g2 h2 e2 <<< "$(acct "$C1" "$WORK/b.ast" "$WORK/b.dat" "$WORK/b.s")"

    printf 'pass0\t%s\t%s\t%s\t%s\t%s\n' "$src" "$sz" "$g0" "$h0" "$e0" | tee -a "$OUT"
    printf 'c0\t%s\t%s\t%s\t%s\t%s\n'    "$src" "$sz" "$g1" "$h1" "$e1" | tee -a "$OUT"
    printf 'c1\t%s\t%s\t%s\t%s\t%s\n'    "$src" "$sz" "$g2" "$h2" "$e2" | tee -a "$OUT"

    if [ "$e0" = "0" ] && [ -n "$g0" ] && [ "$g0" -lt "$min0" ] 2>/dev/null; then
        min0=$g0; w0="$src gap=$g0 heap=$h0 bytes=$sz"
    fi
    if [ "$e1" = "0" ] && [ -n "$g1" ] && [ "$g1" -lt "$min1" ] 2>/dev/null; then
        min1=$g1; w1="$src gap=$g1 heap=$h1 bytes=$sz"
    fi
    if [ "$e2" = "0" ] && [ -n "$g2" ] && [ "$g2" -lt "$min2" ] 2>/dev/null; then
        min2=$g2; w2="$src gap=$g2 heap=$h2 bytes=$sz"
    fi

    rm -f "$ROOT$WORK"/b.*
done < <(find "$ROOT/usr/src" -name '*.c' -type f -printf '%s\t%p\n' \
    | sort -rn | cut -f2- | sed "s#^$ROOT##")

{
    echo "worst (minimum gap) per pass:"
    echo "  pass0 (cpp):   $w0"
    echo "  c0    (pass1): $w1"
    echo "  c1    (pass2): $w2"
} | tee "$WORST"

echo "done. data: $OUT, summary: $WORST"
