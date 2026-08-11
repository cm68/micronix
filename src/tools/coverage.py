#!/usr/bin/env python3
"""Coverage of /bin/sh against the C reconstruction.

Every Whitesmiths function opens by calling a frame helper, so the
function starts are findable exactly.  Size each one, then mark the
ones the reconstruction accounts for.
"""
import re, collections

L = open('sh.dis', errors='replace').read().split('\n')
lab = re.compile(r'^H([0-9a-f]{4}):')
adr = re.compile(r';\s*([0-9a-f]{4}) ')
# A function is a thing that gets CALLed.  Leaves invariably are, so
# that set alone is the answer, plus the entry point because nothing
# calls it.  Detecting starts by "opens with a frame helper" instead
# was wrong BOTH ways on /bin/sh - it invented 38 functions that were
# really string labels and missed 42 real leaves.
cal2 = re.compile(r'CALL\s+H([0-9a-f]{4})')
starts = set()
for l in L:
    for mc in cal2.finditer(l):
        starts.add(int(mc.group(1), 16))
starts.add(0x0100)
starts = sorted(starts)

TEXT_END = 0x0100 + 0x883d
sizes = {}
for i, a in enumerate(starts):
    end = starts[i + 1] if i + 1 < len(starts) else TEXT_END
    sizes[a] = end - a

# What the reconstruction accounts for, and what it is in sh.c/parse.c.
KNOWN = {
    0x011f: "main - argument handling, startup files",
    0x015d: "the read-eval loop",
    0x1c7b: "startup: login flag, source stack seeded",
    0x2c12: "read one line (fgets)",
    0x5f06: "resume a popped input source",
    0x17d7: "TOKENISER - the eight operators",
    0x2257: "PARSER - commands and redirections",
    0x772d: "string equality",
    0x783d: "char-in-set test",
    0x1228: "called from the engine, unidentified",
    0x48dc: "builtin lookup",
    0x4287: "pre-fork setup",
    0x2aa2: "backquote substitution - NOT implemented",
    0x16f2: "fatal error: print and exit",
    0x0a14: "execution engine",
    0x07a8: "dobuiltin - holds every builtin body",
    0x5897: "subshell - re-enters the read-eval loop",
    0x57c4: "redirection",
    0x1285: "two string tests and two messages",
    0x7e5f: "execv wrapper",
    0x4287: "ignore SIGINT and SIGQUIT",
}
# Many CALL targets are not program functions at all - they are the
# compiler's own helpers (the frame setup, the switch dispatcher, the
# arithmetic it cannot open code) and the library.  Counting those as
# shell code inflates the denominator.
#
# The helpers have a signature: called from everywhere and calling
# nothing.  That is mechanical, so use it rather than an address.
callers = collections.defaultdict(set)
outdeg = collections.defaultdict(set)
cur = None
for l in L:
    m = adr.search(l)
    if not m:
        continue
    a = int(m.group(1), 16)
    if a in starts:
        cur = a
    if cur is None:
        continue
    for mc in cal2.finditer(l):
        t = int(mc.group(1), 16)
        callers[t].add(cur)
        outdeg[cur].add(t)

HELPER_CALLERS = 6      # called from at least this many places
helpers = set(a for a in starts
              if len(callers[a]) >= HELPER_CALLERS and not outdeg[a])

# The library is linked ABOVE all the program's own code - so every
# library function sits at a higher address than every program one.
# That is true, and it bounds the answer, but it does NOT give a
# mechanical test on its own: I tried "the library never calls
# downwards across the boundary" and it fails, because library
# functions call each other downwards constantly.  H876f calls H8634.
# Both attempts at deriving it that way gave nonsense - one program
# function, then a hundred and seventy-one.
#
# So the line is set from the lowest routine actually IDENTIFIED as
# library: H7117 is exit.  Everything above it is library or helper.
# That is an anchor rather than a derivation, and it is a bound: the
# real boundary is at or below this, so the shell figure below is an
# over-estimate, not an under-estimate.
LIBC_FROM = 0x7117

shell = [a for a in starts if a < LIBC_FROM and a not in helpers]
libc = [a for a in starts if a >= LIBC_FROM or a in helpers]

sb = sum(sizes[a] for a in shell)
lb = sum(sizes[a] for a in libc)
kb = sum(sizes[a] for a in shell if a in KNOWN)

print("functions: %d total, %d shell, %d library or helper "
      "(%d compiler helpers; library starts at H%04x)" %
      (len(starts), len(shell), len(libc), len(helpers), LIBC_FROM))
print("bytes:     %d shell, %d libc, %d text total" % (sb, lb, sb + lb))
print()
print("accounted for: %d of %d shell functions, %d of %d bytes (%.0f%%)" %
      (len([a for a in shell if a in KNOWN]), len(shell), kb, sb, 100.0 * kb / sb))
print()
print("=== the twenty biggest shell functions NOT accounted for ===")
un = [a for a in shell if a not in KNOWN]
un.sort(key=lambda a: -sizes[a])
for a in un[:20]:
    print("   H%04x  %5d bytes" % (a, sizes[a]))
print()
print("   ... and %d smaller ones, %d bytes in all" %
      (len(un) - 20, sum(sizes[a] for a in un[20:])))
