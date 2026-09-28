#
# The per-disk tables are keyed "disk<FS>name", where <FS> is the byte
# awk itself puts between the parts of a multi-dimensional subscript
# (octal 034).  ent[k "\034" p] and ent[k, p] are the same array, but
# gawk 5.2.1 dies on the second spelling - "internal error: file
# interpret.h, line 263: unexpected parameter type Node_val", then a
# segfault - so the first is written out.  The separator has to be the
# literal as well: a variable holding it, and even the built-in SUBSEP,
# both re-trigger the fault.  Verified against mawk, nawk and busybox
# awk on the same manifest.
#
function fcost(n) {
	if (n <= 0) return 0
	if (n <= 8) return n
	if (n <= 1792) return n + 1
	return n + 2 + int((n - 1792 + 255) / 256)
}
#
# Blocks a directory occupies at a given entry count.  The dot and
# dot-dot entries are two of them.  The root block is one of the two the
# superblock does not use, so it is already paid for and never charged.
#
function nb(d, e,   c) {
	c = int((e + 2 + 31) / 32)
	if (d == ".") c = c - 1
	return c < 0 ? 0 : c
}
#
# The chain from d up to the root, d first, the root-most ancestor last.
#
function chainof(d,   n, p, q) {
	n = 0
	p = d
	while (p != ".") {
		ch[++n] = p
		q = p
		sub(/\/[^\/]*$/, "", q)
		if (q == "" || q == p) p = "."
		else p = q
	}
	return n
}
function walk(k, d, f, docommit,   n, i, a, p, delta, key, t) {
	n = chainof(d)
	delta = 0
	#
	# newdirs is an out parameter: the directories this call would
	# create, counted whether or not docommit is set.  It is cleared
	# here rather than by the caller so that pricing a placement after
	# pricing the empty disk does not count the same directories twice.
	#
	newdirs = 0
	for (i = n; i >= 1; i--) {
		a = ch[i]
		if (i == n) p = "."
		else p = ch[i + 1]
		key = k "\034" a
		if (!(key in pres)) {
			delta += nb(a, 0)
			if (docommit) {
				pres[key] = 1
				ent[k "\034" a] = 0
				ino[k]++
			}
			newdirs++
			#
			# One more entry in the parent, so the parent grows by a
			# block exactly when the count crosses a multiple of 32 -
			# nb(e+1) - nb(e), the -1 for the root cancelling out.
			#
			t = ent[k "\034" p]
			delta += int((t + 34) / 32) - int((t + 33) / 32)
			if (docommit) ent[k "\034" p]++
		}
	}
	t = ent[k "\034" d]
	delta += nb(d, t + f) - nb(d, t)
	if (docommit) ent[k "\034" d] += f
	return delta
}
{
	size = $1 + 0
	path = $2
	if (path == "") next
	d = path
	if (sub(/\/[^\/]*$/, "", d) == 0) d = "."
	if (!(d in seen)) { seen[d] = 1; order[++nu] = d }
	#
	# ublk is the whole unit in data blocks, which is what the "does
	# it fit on a disk of its own" question is asked with.  ufc is the
	# same cost per file, which is what the split path needs: walk
	# charges directories and never data.
	#
	cost = fcost(int((size + 511) / 512))
	ublk[d] += cost
	ufile[d "\034" ++un[d]] = path
	ufc[d "\034" un[d]] = cost
}
END {
	disk = 1
	for (i = 1; i <= nu; i++) {
		d = order[i]
		ec = ublk[d] + walk(-1, d, un[d], 0)
		if (ec > BUDGET || newdirs + un[d] > IBUDGET) {
			#
			# The unit will not fit on a disk of its own, so it goes a
			# file at a time.  /bin is 4200 blocks against 2151 on the
			# eight inch and every large directory is over on the five
			# inch, so this is the normal path there and not a corner.
			#
			for (j = 1; j <= un[d]; j++) {
				delta = ufc[d "\034" j] + walk(disk, d, 1, 0)
				if ((blk[disk] + delta > BUDGET ||
				     ino[disk] + newdirs + 1 > IBUDGET) && blk[disk] > 0) {
					disk++
					delta = ufc[d "\034" j] + walk(disk, d, 1, 0)
				}
				nd = newdirs
				walk(disk, d, 1, 1)
				blk[disk] += delta
				ino[disk] += nd + 1
				print ufile[d "\034" j] > (dir "/list." disk)
			}
			continue
		}
		delta = ublk[d] + walk(disk, d, un[d], 0)
		if (blk[disk] + delta > BUDGET ||
		    ino[disk] + newdirs + un[d] > IBUDGET) {
			disk++
			delta = ublk[d] + walk(disk, d, un[d], 0)
		}
		nd = newdirs
		walk(disk, d, un[d], 1)
		blk[disk] += delta
		ino[disk] += nd + un[d]
		for (j = 1; j <= un[d]; j++)
			print ufile[d "\034" j] > (dir "/list." disk)
	}
	ndisk = disk
	for (k = 1; k <= ndisk; k++)
		printf "disk %d: %5d blocks (%d%%), %5d inodes (%d%%)\n", \
		    k, blk[k] + 0, 100 * blk[k] / BUDGET, \
		    ino[k] + 0, 100 * ino[k] / IBUDGET
	printf "TOTAL %d disks\n", ndisk
}
