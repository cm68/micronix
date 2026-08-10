#!/usr/bin/env python3
"""Cut one page out of a scanned manual's text layer.

    tools/extract.py scan.txt name section [outfile]

A page runs from its own first header to the header of the next
DIFFERENT page, so continuation sheets are absorbed.  Running headers
and -N- footers are dropped.

ALWAYS look at the tail of what comes out.  A page whose header
scanned badly - "l~b (1) OPTION", "printers (41" - is invisible to
the boundary detector, and the page before it swallows the lot.
"""
import sys, os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manlib


def extract(lines, name, sec):
    marks = manlib.headers(lines)
    starts = [k for k, (i, n, s) in enumerate(marks) if n == name and s == sec]
    if not starts:
        return None
    k = starts[0]
    j = k
    while j + 1 < len(marks) and marks[j + 1][1] == name and marks[j + 1][2] == sec:
        j += 1
    begin = marks[k][0]
    end = marks[j + 1][0] if j + 1 < len(marks) else len(lines)

    body = []
    for l in lines[begin:end]:
        s = l.replace('\f', '').rstrip()
        if manlib.is_header(s) or manlib.FOOT.match(s):
            continue
        body.append(s)
    while body and not body[0].strip():
        body.pop(0)
    while body and not body[-1].strip():
        body.pop()
    return body


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    lines = manlib.read_text(argv[0])
    body = extract(lines, argv[1].lower(), argv[2])
    if body is None:
        sys.exit("no page %s(%s) in that text" % (argv[1], argv[2]))
    out = "\n".join(body) + "\n"
    if len(argv) > 3:
        open(argv[3], 'w').write(out)
        print("wrote %s, %d lines" % (argv[3], len(body)))
    else:
        sys.stdout.write(out)


if __name__ == '__main__':
    main(sys.argv[1:])
