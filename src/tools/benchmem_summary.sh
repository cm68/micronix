#!/bin/bash
# benchmem_summary.sh - the worst offenders, from a finished benchmem.tsv.
#
# Reads the tab-separated output of benchmem.sh and prints, per pass,
# the sources that came closest to running out of memory (smallest gap).
# Re-runnable against a saved benchmem.tsv without re-running the
# benchmark, so a summary can be regenerated at will.
#
# Usage:
#   src/tools/benchmem_summary.sh [benchmem.tsv] [N]
#
#   N is how many sources per pass to list (default 15).
#
# The tsv columns are:  pass source bytes gap heap exit
# where gap is the free memory between the break and the stack low-water
# mark - the number that matters.  A gap of "-" means that pass failed
# or timed out on that source (exit != 0), and it is left out.

set -u

TSV=${1:-benchmem.tsv}
N=${2:-15}

[ -f "$TSV" ] || { echo "no such file: $TSV" >&2; exit 1; }

for pass in pass0 c0 c1; do
    case "$pass" in
        pass0) name="cpp (pass0)";;
        c0)    name="pass1 (c0)";;
        c1)    name="pass2 (c1)";;
    esac
    echo
    echo "=== $name: worst offenders (smallest gap) ==="
    printf '  %6s %7s %9s  %s\n' gap heap bytes source
    awk -F'\t' -v p="$pass" '
        $1 == p && $6 == "0" && $4 != "-" && $4 ~ /^[0-9]+$/ {
            print $4, $5, $3, $2
        }
    ' "$TSV" | sort -n | head -n "$N" |
        awk '{ printf "  %6s %7s %9s  %s\n", $1, $2, $3, $4 }'
done
echo
