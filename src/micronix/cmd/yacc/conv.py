import re, sys

# Whitesmith's "natural" C -> standard C for ccc.
#
# Only three shapes are touched, each unambiguous:
#   scalar brace   TYPE name {expr} ;        -> TYPE name = expr ;
#   array brace    TYPE name[N] {a,b} ;      -> TYPE name[N] = {a,b} ;
#   bare init      TYPE name NUMBER|_MACRO ; -> TYPE name = NUMBER|_MACRO ;
#
# A plain "TYPE name ;" is left alone: the value slot must be a number
# or a _-prefixed macro, never a bare identifier (that identifier is the
# declarator's name).  Identifier-valued inits (mem0, C) are fixed by
# hand after this pass.

TYPE_NAME = r'\s*(?:struct\s+)?\w+\s*\*?\s*\w+'

def fixline(s):
    m = re.match(r'^(' + TYPE_NAME + r')\s*\{([^},]+)\}\s*;(.*)$', s)
    if m:
        return f"{m.group(1)} = {m.group(2).strip()};{m.group(3)}\n"
    m = re.match(r'^(' + TYPE_NAME + r'\s*\[[^\]]*\])\s*\{([^}]*)\}\s*;(.*)$', s)
    if m:
        return f"{m.group(1)} = {{{m.group(2).strip()}}};{m.group(3)}\n"
    m = re.match(r'^(' + TYPE_NAME + r')\s+(-?[0-9][0-9]*|_[A-Za-z0-9_]+)\s*;(.*)$', s)
    if m:
        return f"{m.group(1)} = {m.group(2)};{m.group(3)}\n"
    return s

for fn in sys.argv[1:]:
    out = [fixline(l) for l in open(fn).readlines()]
    open(fn, 'w').writelines(out)
