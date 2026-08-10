#!/usr/bin/env python3
"""List the man pages in a scanned manual, and say which are missing.

    pdftotext -layout scan.pdf scan.txt
    tools/index.py scan.txt [/path/to/usr/man]

Lettered subsections count: the library pages are "fopen (3s)", and a
pattern that insists on a bare digit finds none of them.
"""
import sys, os, collections

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manlib


def main(argv):
    if not argv:
        sys.exit(__doc__)
    txt = manlib.read_text(argv[0])
    disk = argv[1] if len(argv) > 1 else None

    pages = collections.OrderedDict()
    for i, name, sec in manlib.headers(txt):
        pages.setdefault((name, sec), []).append(i)

    bysec = collections.defaultdict(list)
    for (n, s) in pages:
        bysec[s].append(n)

    print("distinct pages in the scan: %d" % len(pages))
    print()
    for s in sorted(bysec):
        print("(%s): %d" % (s, len(bysec[s])))
        print("   ", " ".join(sorted(bysec[s])))
        print()

    if not disk:
        return
    have = set()
    for d in sorted(os.listdir(disk)):
        full = os.path.join(disk, d)
        if os.path.isdir(full):
            for f in os.listdir(full):
                have.add(f.rsplit('.', 1)[0].lower())
    print("=== in the scan, no file of that name anywhere on disk ===")
    total = 0
    for s in sorted(bysec):
        miss = sorted(n for n in bysec[s] if n not in have)
        total += len(miss)
        if miss:
            print("(%s) %2d: %s" % (s, len(miss), " ".join(miss)))
    print()
    print("total: %d" % total)
    print()
    print("Check these by eye before reversing any of them: OCR turns")
    print("cp1 into cpl, ls into Is and ln into In, so a name that looks")
    print("missing is often one you already have.")


if __name__ == '__main__':
    main(sys.argv[1:])
