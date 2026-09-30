#!/bin/sh
#
# The memory footprint of the self-host, measured, with a floor.
#
# All four simulated passes - pass0, c0, c1 and peep - compile every one
# of the compiler's own sources, the set that defines the tipping
# point, and each run's brk/gap is read out of the simulator's -S
# report.  Correctness rides along: every pass must produce
# byte-identical output to its host counterpart (pass0 is compared with
# matching relative paths, because the spelling of the source path
# lands in the line markers).
#
# Failure means one of:
#   - a pass ran OUT OF MEMORY on a source it used to fit
#   - a gap fell below the floor (default 256 bytes): still fitting,
#     but a bug fix away from not
#   - simulated output diverged from the host's
#
# The full gap table goes to stdout so the drift is visible long
# before the floor is hit.
#
# Sources are measured in parallel.  Each one is a separate errand -
# a source in, a gap and a verdict out - and the simulator spends
# forty seconds on a source, most of it in c1.  The only thing that
# ever made this serial was the scratch names: every source wrote
# s.x and h.ast into one directory.  Named per source instead, the
# whole table is one wave rather than fifty-five.  FPJOBS overrides
# the width.
#
set -e

here=$(cd "$(dirname "$0")" && pwd)
# The compiler tree under test.  footprint.sh lives in its libexec/,
# so the tree is one level up.  COMPILER_ROOT points it somewhere
# else, the way the fork's copy let it.
mxroot=${COMPILER_ROOT:-$(cd "$here/.." && pwd)}
# The host passes the simulated .mx ones are checked against live in
# the top of the tree, ../.. up from mxroot, in libexec/.
mxhost=$mxroot/../../libexec
# The machine the measurement runs on: the userland simulator, built
# beside the compiler tree in ../usersim.
SIMBIN=$mxroot/../usersim/sim
FLOOR=${FLOOR:-256}
work=${FPWORK:-$here/fpwork}
JOBS=${FPJOBS:-$( n=$(nproc 2>/dev/null || echo 4); echo $((n * 2)) )}

# stage: sources laid out the way the sim's chroot sees them
rm -rf "$work"
base=$(dirname "$work")
out="$work-out"
rm -rf "$out"; mkdir -p "$out"
mkdir -p "$work/pass0" "$work/c0" "$work/c1" "$work/inc" "$work/lib"
for d in pass0 c0 c1; do
	cp "$mxroot"/libexec/$d/*.c "$mxroot"/libexec/$d/*.h "$work/$d/" 2>/dev/null || true
done
# The staging above is the whole test vector, so an empty one measures
# nothing and says everything fits.  It read the ccc fork's sources
# until the tree moved under src/, and the "|| true" that lets a
# directory without headers through hid it: fifty-five sources became
# none and the table came out empty rather than wrong.
for d in pass0 c0 c1; do
	set -- "$work/$d"/*.c
	[ -f "$1" ] || { echo "footprint: no sources staged for $d" >&2; exit 1; }
done
cp "$mxroot"/include/*.h "$work/inc/"
cp -r "$mxroot"/include/sys "$work/inc/" 2>/dev/null || true
cp "$mxroot"/lib/include/*.h "$work/lib/" 2>/dev/null || true
cp -r "$mxroot"/lib/include/sys "$work/lib/" 2>/dev/null || true

# Rebuild every pass from scratch, host and native, before measuring.
# A stale binary silently poisons the comparison - the host peephole once
# sat a commit behind the native one and every source "diverged" that the
# rules had since guarded.  make's timestamps cannot be trusted for the
# host link, so clean and rebuild in SEPARATE invocations: "hostclean
# host" in one run has make decide "nothing to do" before the clean ever
# runs, and the stale binary is never relinked.  Host first: the native
# passes are compiled BY it - the ccc driver execs mxpass0/mxc0/mxc1/mxpeep.
for d in pass0 c0 c1 peep; do
	make -C "$mxroot/libexec/$d" hostclean >/dev/null
	make -C "$mxroot/libexec/$d" host >/dev/null
done
for d in pass0 c0 c1 peep; do
	make -C "$mxroot/libexec/$d" clobber >/dev/null
	make -C "$mxroot/libexec/$d" all >/dev/null
done
# The native binaries, renamed to the .mx names the rest of the script
# invokes them by.
cp "$mxroot/libexec/pass0/pass0" "$work/pass0.mx"
cp "$mxroot/libexec/c0/c0"   "$work/c0.mx"
cp "$mxroot/libexec/c1/c1"   "$work/c1.mx"
cp "$mxroot/libexec/peep/peep" "$work/peep.mx"

export mxroot mxhost work base out FLOOR SIMBIN


# One source: the host chain and the simulated one, compared.  Writes
# its table row and any complaint to out/<tag>, and touches
# out/<tag>.bad if the self-host is at or past the tipping point.
measure() {
	d=${1%/*}; b=${1#*/}
	tag=$2
	# Each source gets its own directory, and the name is three
	# digits wide on purpose: "fpw007" is exactly as long as
	# "fpwork", so the simulator's argv is the same length it was
	# when this ran one source at a time.  argv sits on the initial
	# stack and the gap is measured from there - a longer scratch
	# spelling would quietly cost a dozen bytes of the number being
	# reported.
	jd=$(printf '%s/fpw%03d' "$base" "$tag")
	s=s; h=h; o="$out/$tag.o"
	res="$out/$tag"
	: >"$res"
	gapof() { grep -o 'gap=[0-9-]*' "$1" 2>/dev/null | tail -1 | cut -d'=' -f2; }
	bad() { echo "$1" >>"$res"; : >"$out/$tag.bad"; }

	rm -rf "$jd"; cp -al "$work" "$jd" 2>/dev/null || cp -a "$work" "$jd"
	SIM="$SIMBIN -S -d $jd"

	# The host passes, named as hostcc installs them.  -Ilib and
	# -Iinc carry the C-library and system headers the way the tree's
	# own cross build lays them out; each pass's headers ride beside it.
	# Run each one separately, never chained with &&: c0 recovers from a
	# bad op (it reports "undefined symbol" and goes on to write a valid
	# .ast) but exits non-zero, and a chain would then skip c1 and leave
	# h.s missing - which the c1 compare read as a divergence that was
	# never there.  The exit code still matters: a recovered error is a
	# bug to fix, not something to bury, so a non-zero exit is flagged
	# the way OOM is on the native side, and the byte comparisons below
	# only run when the host pass actually finished.
	hp0=0; hc0=0; hc1=0
	(cd "$jd" && "$mxhost"/mxpass0 -I$d -Ilib -Iinc -o $h $d/$b.c) \
		>"$o.hpass0" 2>&1 || hp0=$?
	(cd "$jd" && "$mxhost"/mxc0 $h.x $h.ast $h.dat) \
		>"$o.hc0" 2>&1 || hc0=$?
	(cd "$jd" && "$mxhost"/mxc1 $h.ast $h.dat $h.s) \
		>"$o.hc1" 2>&1 || hc1=$?
	[ "$hp0" = 0 ] || bad "$d/$b: pass0 ERROR (host exit $hp0)"
	[ "$hc0" = 0 ] || bad "$d/$b: c0 ERROR (host exit $hc0)"
	[ "$hc1" = 0 ] || bad "$d/$b: c1 ERROR (host exit $hc1)"

	(cd "$jd" && timeout 300 $SIM pass0.mx -I$d -Ilib -Iinc \
		-o $s $d/$b.c </dev/null) >"$o.pass0" 2>&1 || true
	gc=$(gapof "$o.pass0"); : "${gc:=?}"
	if grep -q "out of memory" "$o.pass0"; then
		bad "$d/$b: pass0 OUT OF MEMORY (the tipping point)"
		printf '%-18s %-13s %-13s %-13s %-13s\n' "$d/$b" OOM - - - >>"$res"
		return 0
	fi
	if [ "$hp0" = 0 ]; then
		cmp -s "$jd/$s.x" "$jd/$h.x" || bad "$d/$b: pass0 DIVERGES"
	fi

	(cd "$jd" && timeout 300 $SIM c0.mx $s.x $s.ast $s.dat \
		</dev/null) >"$o.c0" 2>&1 || true
	g0=$(gapof "$o.c0"); : "${g0:=?}"
	if grep -q "out of memory" "$o.c0"; then
		bad "$d/$b: c0 OUT OF MEMORY (the tipping point)"
		printf '%-18s %-13s %-13s %-13s %-13s\n' "$d/$b" "$gc" OOM - - >>"$res"
		return 0
	fi
	if [ "$hc0" = 0 ]; then
		cmp -s "$jd/$s.ast" "$jd/$h.ast" && cmp -s "$jd/$s.dat" "$jd/$h.dat" ||
			bad "$d/$b: c0 DIVERGES"
	fi

	g1=-
	gp=-
	# The ast alone decides whether there is anything to generate.  An
	# empty .dat is what a source with no initialized data produces -
	# tparse, pfx, post and rules among them - and requiring it to be
	# non-empty quietly dropped fourteen of the fifty-one sources out
	# of the c1 and peep columns, including the largest one in the
	# tree.  They printed "-" and read as nothing to see.
	if [ -s "$jd/$s.ast" ]; then
		(cd "$jd" && timeout 600 $SIM c1.mx $s.ast $s.dat $s.s \
			</dev/null) >"$o.c1" 2>&1 || true
		g1=$(gapof "$o.c1"); : "${g1:=?}"
		if grep -q "out of memory" "$o.c1"; then
			bad "$d/$b: c1 OUT OF MEMORY"; g1=OOM
		else
			if [ "$hc1" = 0 ]; then
				grep -v '^;' "$jd/$s.s" >"$jd/$s.cmp" 2>/dev/null || true
				grep -v '^;' "$jd/$h.s" >"$jd/$h.cmp" 2>/dev/null || true
				cmp -s "$jd/$s.cmp" "$jd/$h.cmp" ||
					bad "$d/$b: c1 DIVERGES"
			fi
		fi
	fi

	# peep, on what c1 just wrote.  It is the fourth pass that has to
	# fit, and the only one whose input is the OUTPUT of the biggest
	# one - so the sources that stretch c1 are the ones that hand peep
	# its largest input too, and both ends want watching on the same
	# row.
	#
	# Both peeps are given the SAME file - the one c1.mx just wrote -
	# rather than each being given its own chain's output.  peep is the
	# one pass where that distinction bites.  The host passes are built
	# -DDEBUG and the .mx ones are not, so the host c1 writes "; stmt"
	# and "; FUNC" lines that c1.mx never emits, and classify() calls
	# anything without a colon before a space an instruction - so those
	# comment lines sit in the window and block matches that fire
	# without them.  The two peeps then differ in real instructions,
	# not just comments, and no amount of stripping afterwards can
	# separate that from peep being wrong.  On one input they agree
	# exactly, which is the question worth asking: does peep.mx do what
	# peep does?
	if [ -s "$jd/$s.s" ]; then
		(cd "$jd" && "$mxhost"/mxpeep $s.s $h.p) >/dev/null 2>&1 || true
		(cd "$jd" && timeout 600 $SIM peep.mx $s.s $s.p \
			</dev/null) >"$o.pp" 2>&1 || true
		gp=$(gapof "$o.pp"); : "${gp:=?}"
		if grep -q "out of memory" "$o.pp"; then
			bad "$d/$b: peep OUT OF MEMORY"; gp=OOM
		else
			cmp -s "$jd/$s.p" "$jd/$h.p" || bad "$d/$b: peep DIVERGES"
		fi
	fi

	printf '%-18s %-13s %-13s %-13s %-13s\n' \
		"$d/$b" "$gc" "$g0" "$g1" "$gp" >>"$res"
	for g in "$gc" "$g0" "$g1" "$gp"; do
		case "$g" in -|OOM|"?") continue;; esac
		if [ "$g" -lt "$FLOOR" ]; then
			bad "$d/$b: gap $g under the $FLOOR-byte floor"
		fi
	done
	[ -z "$KEEP" ] && { rm -rf "$jd"; rm -f "$o".*; }
	return 0
}
list=""
for d in pass0 c0 c1; do
	for f in "$work"/$d/*.c; do
		b=$(basename "$f" .c)
		[ "$b" = test ] && continue
		list="$list $d/$b"
	done
done

printf '%-18s %-13s %-13s %-13s %-13s\n' source "pass0 gap" "c0 gap" "c1 gap" "peep gap"

# a batch at a time, JOBS wide.  Every source is independent now that
# the scratch names carry its tag, so the only thing to wait for is
# the slowest one in each batch.
n=0; i=0
for one in $list; do
	i=$((i + 1))
	measure "$one" "$i" &
	n=$((n + 1))
	if [ "$n" -ge "$JOBS" ]; then wait; n=0; fi
done
wait

fail=0; i=0
for one in $list; do
	i=$((i + 1)); tag=$i
	[ -f "$out/$tag" ] && cat "$out/$tag"
	[ -f "$out/$tag.bad" ] && fail=1
done

[ -z "$KEEP" ] && rm -rf "$work" "$out" "$base"/fpw[0-9][0-9][0-9]
if [ "$fail" = 0 ]; then
	echo "footprint: every pass fits every source, no gap under $FLOOR"
else
	echo "footprint: FAILED - the self-host is at or past the tipping point"
fi
exit $fail
