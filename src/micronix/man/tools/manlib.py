#!/usr/bin/env python3
"""Shared bits for reversing scanned man pages.

A page top is a form(1) .he line as it came out of the printer:

    name (n)            4/6/83            name (n)

the same name at both ends.  Cross-reference lines look similar
("newuser (1), chsh (1), ... owner (1)") and must NOT be mistaken for
one - matching the shape alone ate the SEE ALSO off account(1).
"""
import re

HDR = re.compile(r'^\s*\f?\s*([A-Za-z_][A-Za-z0-9_.]*)\s*\(([1-8][a-z]?)\)(.*?)'
                 r'([A-Za-z_][A-Za-z0-9_.]*)\s*\(\2\)\s*$')
FOOT = re.compile(r'^\s*-\s*[\d?]+\s*-\s*$')
DATE = re.compile(r'(\d{1,2}/\d{1,2}/\d{2})')

# every request form(1) actually implements
REQUESTS = set("bd bp br ce fi fo he in ls nf pl rm sp ta ti ul".split())


def is_header(line):
    """(name, section, middle) if this line is a running header."""
    m = HDR.match(line)
    if not m:
        return None
    if m.group(1).lower() != m.group(4).lower():
        return None
    if len(m.group(3).strip()) > 40:
        return None
    return m.group(1).lower(), m.group(2), m.group(3)


def headers(lines):
    """Every running header, in order: (line number, name, section)."""
    out = []
    for i, l in enumerate(lines):
        h = is_header(l)
        if h:
            out.append((i, h[0], h[1]))
    return out


def read_text(path):
    return open(path, errors='replace').read().split('\n')


def furniture(name, sec, date, width=65):
    """The .he/.fo pair, padded the way the originals are.

    The real pages are not perfectly centred - null.4 splits 20/22
    around the date, and 19 of them pad with tabs - so this is a
    reasonable house average rather than an exact reproduction.
    """
    tag = "%s (%s)" % (name, sec)
    gap = width - 2 * len(tag) - len(date)
    if gap < 2:
        raise ValueError("name too long for a %d column header" % width)
    left = gap // 2
    he = tag + " " * left + date + " " * (gap - left) + tag
    return ['.he "%s"' % he,
            '.fo "%s"' % (" " * ((width - 3) // 2) + "-#-"),
            '.in 5',
            '.rm %d' % width]


def heading(name):
    """The five lines that start a section, majority idiom."""
    return ['.in -5', '.bd', name, '.in +5', '.br']
