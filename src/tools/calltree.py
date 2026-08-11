#!/usr/bin/env python3
"""Call tree of a disassembled Whitesmiths image.

    calltree.py sh.dis [root]

Every function opens by calling a frame helper, so the starts are
exact rather than guessed.  Edges come from the CALL targets in each
function's own range.  Printed as a tree from the root - main unless
told otherwise - with each function shown once in full and afterwards
by reference, so the output stays the size of the program rather than
the size of its paths.
"""
import re, sys, collections

FRAME = ('H88e2', 'H88ed', 'H34c0', 'H34cb')    # sh's and form's
lab = re.compile(r'^H([0-9a-f]{4}):')
adr = re.compile(r';\s*([0-9a-f]{4}) ')
cal = re.compile(r'CALL\s+H([0-9a-f]{4})')
sysd = re.compile(r'SYS\s+([a-z_0-9]+)')
sysi = re.compile(r'SYS indir ([0-9a-f]{2}) ([0-9a-f]{2})')


def load(path):
    L = open(path, errors='replace').read().split('\n')

    starts = []
    for i, l in enumerate(L):
        m = lab.match(l)
        if m:
            blob = l + (L[i + 1] if i + 1 < len(L) else '')
            if any(f in blob for f in FRAME):
                starts.append(int(m.group(1), 16))
    # A leaf - a syscall stub, say - does not open with a frame
    # helper, so it never shows up above and the function before it
    # swallows it whole.  Every CALL target is a function start too.
    for l in L:
        for mc in cal.finditer(l):
            starts.append(int(mc.group(1), 16))
    starts = sorted(set(starts))

    # walk the listing once, attributing every line to the function it
    # is inside
    edges = collections.defaultdict(list)
    syscalls = collections.defaultdict(set)
    size = {}
    cur = None
    last = {}
    for l in L:
        m = adr.search(l)
        if not m:
            continue
        a = int(m.group(1), 16)
        if a in starts:
            cur = a
        if cur is None:
            continue
        last[cur] = max(last.get(cur, a), a)
        for mc in cal.finditer(l):
            t = int(mc.group(1), 16)
            if t not in edges[cur]:
                edges[cur].append(t)
        mi = sysi.search(l)
        if mi:
            syscalls[cur].add("indir@%s%s" % (mi.group(2), mi.group(1)))
        else:
            md = sysd.search(l)
            if md and md.group(1) != 'indir':
                syscalls[cur].add(md.group(1))
    for i, a in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else last.get(a, a)
        size[a] = end - a
    return starts, edges, size, syscalls


def main(argv):
    if not argv:
        sys.exit(__doc__)
    starts, edges, size, syscalls = load(argv[0])
    root = int(argv[1], 16) if len(argv) > 1 else starts[0]

    seen = set()
    out = []

    def walk(a, depth, path):
        pad = '  ' * depth
        sc = syscalls.get(a)
        tag = ('  <%s>' % ' '.join(sorted(sc))) if sc else ''
        if a in path:
            out.append("%sH%04x  (recurses)" % (pad, a))
            return
        if a in seen:
            out.append("%sH%04x  %d%s  ..." % (pad, a, size.get(a, 0), tag))
            return
        seen.add(a)
        out.append("%sH%04x  %d%s" % (pad, a, size.get(a, 0), tag))
        for t in edges.get(a, []):
            if t in size and ('H%04x' % t) not in FRAME:
                walk(t, depth + 1, path | {a})

    walk(root, 0, frozenset())
    print('\n'.join(out))
    print()
    reached = seen
    unreached = [a for a in starts if a not in reached]
    print("%d functions reached from H%04x, %d not reached (%d bytes)" %
          (len(reached), root, len(unreached),
           sum(size.get(a, 0) for a in unreached)))
    if unreached:
        print("unreached:", ' '.join('H%04x' % a for a in unreached[:24]),
              '...' if len(unreached) > 24 else '')


if __name__ == '__main__':
    main(sys.argv[1:])
