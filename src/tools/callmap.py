#!/usr/bin/env python3
"""Show a routine's extent and what it calls.

A routine runs from its label to the next label that is the target of
a CALL from somewhere else - good enough here, because Whitesmiths
puts every function behind a frame helper, so the real starts are the
labels immediately followed by "CALL <framehelper>".
"""
import re, sys, collections

L = open('sh.dis', errors='replace').read().split('\n')
lab = re.compile(r'^H([0-9a-f]{4}):')
adr = re.compile(r';\s*([0-9a-f]{4}) ')
cal = re.compile(r'CALL\s+H([0-9a-f]{4})')
FRAME = ('H88e2', 'H88ed')

# every function start: a label whose line or next line calls a frame helper
starts = set()
for i, l in enumerate(L):
    m = lab.match(l)
    if not m:
        continue
    blob = l + (L[i+1] if i + 1 < len(L) else '')
    if any(f in blob for f in FRAME):
        starts.add(int(m.group(1), 16))

def routine(addr):
    """lines belonging to the routine starting at addr"""
    out = []
    on = False
    for l in L:
        m = lab.match(l)
        if m:
            a = int(m.group(1), 16)
            if a == addr:
                on = True
            elif on and a in starts:
                break
        if on:
            out.append(l)
    return out

for want in sys.argv[1:]:
    a = int(want, 16)
    body = routine(a)
    calls = collections.Counter()
    lo = hi = None
    for l in body:
        ma = adr.search(l)
        if ma:
            v = int(ma.group(1), 16)
            lo = v if lo is None else min(lo, v)
            hi = v if hi is None else max(hi, v)
        for mc in cal.finditer(l):
            calls[mc.group(1)] += 1
    print("=== H%04x : %d lines, %04x..%04x (%d bytes) ===" %
          (a, len(body), lo or 0, hi or 0, (hi - lo) if lo else 0))
    for t, n in calls.most_common():
        mark = ' *' if int(t, 16) in starts else ''
        print("   calls H%s x%d%s" % (t, n, mark))
    print()
