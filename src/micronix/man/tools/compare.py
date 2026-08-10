#!/usr/bin/env python3
"""How far a batch of reversed pages is from the house style.

    tools/compare.py reversed_dir original_dir

Prints requests per page for both.  The numbers that matter, measured
against the 183 originals:

    .bd     14.8 per page   - bold EVERY identifier in prose, and use
                              .bd n for a multi-line declaration
    .sp      5.2 per page   - only between declaration groups and
                              before a heading (.sp 2)
    blank   11.2 per page   - this is what separates paragraphs

A first pass typically lands at half the .bd and several times the
.sp.  See the README, section 4.
"""
import sys, os, re, glob, collections

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manlib


def profile(d):
    req = collections.Counter()
    blank = 0
    npages = 0
    nlines = 0
    files = sorted(glob.glob(os.path.join(d, 'man*', '*')))
    if not files:
        files = sorted(f for f in glob.glob(os.path.join(d, '*'))
                       if os.path.isfile(f))
    for f in files:
        npages += 1
        for l in manlib.read_text(f):
            nlines += 1
            if l.startswith('.'):
                m = re.match(r'\.([a-zA-Z]+)', l)
                if m:
                    req[m.group(1)] += 1
            elif not l.strip():
                blank += 1
    return npages, nlines, req, blank


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    a = profile(argv[0])
    b = profile(argv[1])
    print("%-10s %12s %12s" % ("per page", os.path.basename(argv[0].rstrip('/')),
                               os.path.basename(argv[1].rstrip('/'))))
    print("%-10s %12d %12d" % ("pages", a[0], b[0]))
    print("%-10s %12.1f %12.1f" % ("lines", a[1] / a[0], b[1] / b[0]))
    keys = set(a[2]) | set(b[2])
    for k in sorted(keys, key=lambda k: -(a[2][k] + b[2][k])):
        print("%-10s %12.1f %12.1f" % ("." + k, a[2][k] / a[0], b[2][k] / b[0]))
    print("%-10s %12.1f %12.1f" % ("blank", a[3] / a[0], b[3] / b[0]))


if __name__ == '__main__':
    main(sys.argv[1:])
