#!/usr/bin/env python3
#
# translate_slr.py - turn the monitor ROM sources (zmac syntax) into SLR
# Systems Z80ASM syntax (.z80).  The ROMs are assembled under upm on
# micronix with the SLR Z80ASM 1.09 from disks/whitesmith - see
# ../../../memory/slr-z80asm-under-upm.md (or BOOTROM.md) for the
# invocation and why each rule below exists.
#
# This is a host tool: run it from anywhere, the ROM sources are found
# relative to this script, and the .z80 files are written alongside the .s.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'micronix', 'stand', 'roms')


def _oct_to_hex(m):
    n = int(m.group(1), 8)
    h = '%X' % n
    if h[0] in 'ABCDEF':
        h = '0' + h
    return h + 'H'


def translate(text):
    out = []
    in_phase = False
    for line in text.split('\n'):
        s = line.strip()
        # zmac-only directives: drop them
        if s in ('.z80', 'aseg'):
            continue
        # a real .phase/.dephase directive (line starts with it)
        if re.match(r'^\s*\.phase\b', line):
            in_phase = True
        elif re.match(r'^\s*\.dephase\b', line):
            in_phase = False
        # the final end: SLR needs .DEPHASE to close an open .PHASE
        if re.match(r'^\s*end\b', line) and in_phase:
            out.append('\t.DEPHASE')
            in_phase = False
        # .phase X -> .PHASE X ; .dephase -> .DEPHASE (SLR keeps the dot)
        line = re.sub(r'^(\s*)\.phase\b', r'\1.PHASE', line)
        line = re.sub(r'^(\s*)\.dephase\b', r'\1.DEPHASE', line)
        # octal literal nnnQ -> hex (zmac trailing-Q octal; SLR has none)
        line = re.sub(r'\b([0-7]+)[Qq]\b', _oct_to_hex, line)
        # directive renames (word-at-a-time).  Labels keep their colon: a
        # bare reserved word (e.g. ENTRY) is a pseudo-op to SLR, the colon
        # is what marks it a label.
        for a, b in (('db', 'DEFB'), ('ds', 'DEFS'), ('dw', 'DEFW'),
                     ('org', 'ORG'), ('equ', 'EQU'), ('end', 'END'),
                     ('title', 'TITLE')):
            line = re.sub(r'\b' + a + r'\b', b, line)
        # hex suffix h -> H (hex literals start with a digit in these sources)
        line = re.sub(r'([0-9][0-9a-fA-F]*)h\b', r'\1H', line)
        out.append(line)
    return '\n'.join(out)


def fix_forward_refs(text):
    # mon447/mon500 reference the register-save area (regsav) from an EQU
    # *before* the data that defines it:  ersav EQU regsav + 2 ... regsav: dw 0.
    # SLR will not resolve a forward reference inside an EQU (even in 2-pass
    # under upm, which cannot seek the source back for pass 2), so ersav
    # falls back to 0.  regsav is at 14h in both - a frozen layout - so
    # declare it up front and drop the data label it would clash with.
    text = re.sub(r'(?m)^([ \t]*)regsav:[ \t]*DEFW\b', r'\1DEFW', text)
    text = re.sub(r'(?m)^([ \t]*)ersav[ \t]+EQU[ \t]+regsav[ \t]*\+[ \t]*2\b',
                  r'regsav\tEQU\t14H\t\t;forward: reg-save area (data below)\n'
                  r'\1ersav\tEQU\tregsav + 2',
                  text)
    return text


for name in ('mon375', 'mon447', 'mon500', 'multIO'):
    with open(os.path.join(SRC, name + '.s')) as f:
        t = f.read()
    t = translate(t)
    if name in ('mon447', 'mon500'):
        t = fix_forward_refs(t)
    with open(os.path.join(SRC, name.lower() + '.z80'), 'w') as f:
        f.write(t)
    print(name.lower() + '.z80')
