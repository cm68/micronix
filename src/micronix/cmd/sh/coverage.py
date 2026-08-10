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
FRAME = ('H88e2', 'H88ed')

starts = []
for i, l in enumerate(L):
    m = lab.match(l)
    if not m:
        continue
    blob = l + (L[i + 1] if i + 1 < len(L) else '')
    if any(f in blob for f in FRAME):
        starts.append(int(m.group(1), 16))
starts = sorted(set(starts))

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
    0x1228: "builtin dispatch (the switch at 0x075e)",
    0x48dc: "builtin lookup",
    0x4287: "pre-fork setup",
    0x2aa2: "backquote substitution - NOT implemented",
    0x16f2: "fatal error: print and exit",
    0x0a14: "execution engine",
    0x07a8: "run one command",
    0x5897: "subshell - re-enters the read-eval loop",
    0x57c4: "redirection",
    0x1285: "two string tests and two messages",
    0x7e5f: "execv wrapper",
    0x4287: "ignore SIGINT and SIGQUIT",
}
# The libc floor: everything at or above the first syscall stub is
# library, not shell.  The stubs start at H8094 (the first indir).
LIBC_FROM = 0x8094

shell = [a for a in starts if a < LIBC_FROM]
libc = [a for a in starts if a >= LIBC_FROM]

sb = sum(sizes[a] for a in shell)
lb = sum(sizes[a] for a in libc)
kb = sum(sizes[a] for a in shell if a in KNOWN)

print("functions: %d total, %d below the libc floor, %d at or above" %
      (len(starts), len(shell), len(libc)))
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
