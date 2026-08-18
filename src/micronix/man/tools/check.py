#!/usr/bin/env python3
"""Validate reversed pages against what form(1) implements.

    tools/check.py dir [dir ...]

Checks that every request is one form(1) knows, and that .he is 65
columns.  It does NOT complain that a page ends at indent 10 - the
last section is never closed, and 134 of the 183 originals end that
way too.
"""
import sys, os, re, glob

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manlib


def check(path):
    lines = manlib.read_text(path)
    notes = []
    for i, l in enumerate(lines, 1):
        if not l.startswith('.'):
            continue
        m = re.match(r'\.([a-zA-Z]+|\.\.)', l)
        if not m:
            notes.append("line %d: bare dot - form will eat this line" % i)
        elif m.group(1) not in manlib.REQUESTS:
            notes.append("line %d: .%s is not a form request" % (i, m.group(1)))
    h = [l for l in lines if l.startswith('.he ')]
    if not h:
        notes.append("no .he")
    else:
        w = len(h[0][5:-1])
        if w != 65:
            notes.append(".he is %d columns, want 65" % w)
    if not any(l.startswith('.fo ') for l in lines):
        notes.append("no .fo")
    return len(lines), notes


def main(argv):
    if not argv:
        sys.exit(__doc__)
    files = []
    for d in argv:
        got = sorted(glob.glob(os.path.join(d, 'man*', '*')))
        if not got:
            # a bare directory of pages, rather than a man tree
            got = sorted(f for f in glob.glob(os.path.join(d, '*'))
                         if os.path.isfile(f))
        files += got
    bad = 0
    for f in files:
        n, notes = check(f)
        if notes:
            bad += 1
            print("%-28s %4d  %s" % (f, n, "; ".join(notes)))
    print()
    print("%d of %d files have problems" % (bad, len(files)))


if __name__ == '__main__':
    main(sys.argv[1:])
