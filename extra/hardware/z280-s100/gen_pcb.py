#!/usr/bin/env python3
"""Generate z280-s100.kicad_pcb — S-100 CPU card, components placed + nets wired.

Board outline + edge-connector geometry copied from the existing Z80 S-100 board
(extra/hardware/s100z80/s100_Z80 V2.brd):

    Di        -3.959861 2.61874 289.671762 171.752261   (mm)
    S100_MALE at (50.8, 157.48), 100 pads, 0.125" pitch, fingers 1.778 x 8.382 mm

Reads z280-s100.net (exported from the schematic) and assigns every pad its net,
so the ratsnest is populated for routing. Footprints are approximate; replace
with library footprints before routing.
"""
import os
import re
import uuid

INCH = 25.4

# ---- board outline (copied from Z80 board `Di`) ----
BX0, BY0 = -3.959861, 2.61874
BX1, BY1 = 289.671762, 171.752261

# ---- S-100 edge connector (copied from S100_MALE) ----
CONN_X, CONN_Y = 50.8, 157.48
PITCH = 0.125 * INCH                  # 3.175 mm
PAD_W, PAD_H = 1.778, 8.382           # finger width x length
PAD_X0 = 2.667                        # local x of pad 1
PAD_Y = -4.699                        # local y of the pad row

# ---- netlist (ref, pin) -> net name ----
NETMAP = {}
NETIDX = {}


def load_netlist(path):
    global NETMAP, NETIDX
    if not os.path.exists(path):
        print("warning: %s not found — pads left on net 0" % path)
        return
    s = open(path).read()
    nets = re.findall(r'\(net \(code "\d+"\) \(name "([^"]*)"\)(.*?)'
                      r'(?=\n    \(net \(code|\Z)', s, re.S)
    for nm, seg in nets:
        for r, p in re.findall(r'\(node \(ref "([^"]+)"\) \(pin "(\d+)"\)', seg):
            NETMAP[(r, int(p))] = nm
    names = sorted(set(NETMAP.values()))
    NETIDX = {n: i + 1 for i, n in enumerate(names)}
    print("loaded %d nets, %d pin assignments" % (len(names), len(NETMAP)))


def netref(ref, num):
    n = NETMAP.get((ref, int(num)))
    if n is None:
        return '(net 0 "")'
    return '(net %d "%s")' % (NETIDX[n], n)


def _u():
    return uuid.uuid4().hex.upper()


# ============================================================================
# pad / footprint emitters
# ============================================================================

def pad_th(ref, num, x, y, size=1.6, drill=0.8):
    return ('    (pad "%s" thru_hole roundrect (at %.4f %.4f) (size %.2f %.2f)'
            ' (drill %.2f) (layers "*.Cu" "*.Mask") %s)'
            % (num, x, y, size, size, drill, netref(ref, num)))


def pad_smd(ref, num, x, y, w, h, layers):
    return ('    (pad "%s" smd rect (at %.4f %.4f) (size %.2f %.2f)'
            ' (layers %s) %s)'
            % (num, x, y, w, h, layers, netref(ref, num)))


def fp_open(name, ref, value, x, y, rot=0):
    return ('  (footprint "%s" (layer "F.Cu") (at %.4f %.4f %d)\n'
            '    (property "Reference" "%s" (at 0 0 0) (layer "F.SilkS")\n'
            '      (effects (font (size 1 1))))\n'
            '    (property "Value" "%s" (at 0 0 0) (layer "F.Fab")\n'
            '      (effects (font (size 1 1))))'
            % (name, x, y, rot, ref, value))


def dip_fp(ref, value, n, x, y, wide=False, rot=0):
    """Through-hole DIP: 2 rows of n/2, 2.54 mm pitch. Pin 1 top-left."""
    row = (15.24 if wide else 7.62) / 2.0
    half = n // 2
    L = [fp_open("DIP-%d" % n, ref, value, x, y, rot)]
    for i in range(half):
        py = (half - 1) / 2.0 * 2.54 - i * 2.54
        L.append(pad_th(ref, str(i + 1), -row, py))
        L.append(pad_th(ref, str(i + 1 + half), row, -py))
    L.append("  )")
    return "\n".join(L)


def plcc_fp(ref, value, n, x, y, rot=0):
    """PLCC (J-lead) SMD, 4-sided, 1.27 mm pitch. Approx geometry."""
    per = n // 4
    span = (per - 1) * 1.27 / 2.0
    h = span + 1.5
    L = [fp_open("PLCC-%d" % n, ref, value, x, y, rot)]
    k = 1
    for i in range(per):
        d = -span + i * 1.27
        L.append(pad_smd(ref, str(k), d, -h, 1.6, 0.8, '"F.Cu" "F.Mask"')); k += 1
        L.append(pad_smd(ref, str(k), d, h, 1.6, 0.8, '"F.Cu" "F.Mask"')); k += 1
        L.append(pad_smd(ref, str(k), -h, d, 0.8, 1.6, '"F.Cu" "F.Mask"')); k += 1
        L.append(pad_smd(ref, str(k), h, d, 0.8, 1.6, '"F.Cu" "F.Mask"')); k += 1
    L.append("  )")
    return "\n".join(L)


def hdr_fp(ref, value, n, x, y, rot=0, pitch=2.54):
    """Through-hole inline part (TO-220/TO-92/crystal/header/resistor)."""
    L = [fp_open("HDR-%d" % n, ref, value, x, y, rot)]
    half = (n - 1) * pitch / 2.0
    for i in range(n):
        L.append(pad_th(ref, str(i + 1), -half + i * pitch, 0))
    L.append("  )")
    return "\n".join(L)


def edge_connector():
    """100-pad S-100 card edge (50 front + 50 back at 0.125 in pitch)."""
    L = ['  (footprint "S100_MALE" (layer "F.Cu") (at %.4f %.4f)'
         % (CONN_X, CONN_Y),
         '    (property "Reference" "J1" (at 0 -11.43 0) (layer "F.SilkS")'
         '      (effects (font (size 1.524 1.524))))',
         '    (property "Value" "S-100 edge" (at 0 -11.43 0) (layer "F.Fab")'
         '      (effects (font (size 1.524 1.524))))']
    for i in range(50):
        x = PAD_X0 + i * PITCH
        w = 2.794 if i == 0 else PAD_W
        L.append('    (pad "%d" smd rect (at %.4f %.4f) (size %.2f %.2f)'
                 ' (layers "F.Cu" "F.Mask") %s)'
                 % (i + 1, x, PAD_Y, w, PAD_H, netref("J1", i + 1)))
        L.append('    (pad "%d" smd rect (at %.4f %.4f) (size %.2f %.2f)'
                 ' (layers "B.Cu" "B.Mask") %s)'
                 % (i + 51, x, PAD_Y, w, PAD_H, netref("J1", i + 51)))
    L.append("  )")
    return "\n".join(L)


# ============================================================================
# component placement: (ref, value, kind, p1, p2, x, y, rot)
#   kind "plcc": p1 = pin count
#   kind "dip":  p1 = pin count, p2 = wide (1) / narrow (0)
#   kind "hdr":  p1 = pin count, p2 = pitch (mm)
# ============================================================================

COMPONENTS = [
    ("U1",  "Z280 (PLCC-68)",   "plcc", 68, 0,    70,  100, 0),
    ("U2",  "ATF1508 control",  "plcc", 84, 0,    130, 112, 0),
    ("U24", "ATF1508 data",     "plcc", 84, 0,    130, 62,  0),
    ("U3",  "74HC573 lo",       "dip",  20, 0,    48,  128, 0),
    ("U4",  "74HC573 hi",       "dip",  20, 0,    48,  96,  0),
    ("U9",  "SRAM bank0 even",  "dip",  32, 1,    188, 112, 0),
    ("U10", "SRAM bank0 odd",   "dip",  32, 1,    188, 78,  0),
    ("U22", "SRAM bank1 even",  "dip",  32, 1,    216, 112, 0),
    ("U23", "SRAM bank1 odd",   "dip",  32, 1,    216, 78,  0),
    ("U11", "27SF020 even",     "dip",  32, 1,    250, 112, 0),
    ("U12", "27SF020 odd",      "dip",  32, 1,    250, 78,  0),
    ("U16", "74HCT245 addr0",   "dip",  20, 0,    62,  28,  0),
    ("U17", "74HCT245 addr1",   "dip",  20, 0,    82,  28,  0),
    ("U18", "74HCT245 addr2",   "dip",  20, 0,    102, 28,  0),
    ("U19", "74HCT245 status",  "dip",  20, 0,    122, 28,  0),
    ("U20", "74HCT245 control", "dip",  20, 0,    142, 28,  0),
    ("U21", "74HCT245 pHLDA",   "dip",  20, 0,    162, 28,  0),
    ("U15", "7805",             "hdr",  3,  2.54, 20,  52,  0),
    ("U14", "DS1813 reset",     "hdr",  3,  1.27, 20,  32,  0),
    ("Y1",  "24 MHz crystal",   "hdr",  2,  4.83, 20,  76,  0),
    ("U13", "MAX232",           "dip",  16, 0,    20,  100, 0),
    ("R1",  "1k pRDY",          "hdr",  2,  7.62, 262, 30,  0),
    ("R2",  "1k XRDY",          "hdr",  2,  7.62, 262, 40,  0),
    ("R3",  "1k SLAVE_ONLY",    "hdr",  2,  7.62, 262, 50,  0),
    ("R4",  "1k SIXTN",         "hdr",  2,  7.62, 262, 60,  0),
    ("J2",  "JTAG (1x6)",       "hdr",  6,  2.54, 35,  28,  0),
    ("J3",  "JP VPP",           "hdr",  2,  2.54, 252, 40,  0),
    ("J4",  "JP A15",           "hdr",  2,  2.54, 252, 45,  0),
    ("J5",  "JP A16",           "hdr",  2,  2.54, 252, 50,  0),
    ("J6",  "JP A17/VDD",       "hdr",  2,  2.54, 252, 55,  0),
    ("J7",  "JP PGM",           "hdr",  2,  2.54, 252, 60,  0),
    ("J8",  "JP VDD",           "hdr",  2,  2.54, 252, 65,  0),
]


def build_comp(comp):
    ref, value, kind, p1, p2, x, y, rot = comp
    if kind == "plcc":
        return plcc_fp(ref, value, p1, x, y, rot)
    if kind == "dip":
        return dip_fp(ref, value, p1, x, y, bool(p2), rot)
    return hdr_fp(ref, value, p1, x, y, rot, p2)


def board_outline():
    def seg(x0, y0, x1, y1):
        return ('  (gr_line (start %.4f %.4f) (end %.4f %.4f)'
                ' (layer "Edge.Cuts") (width 0.1))' % (x0, y0, x1, y1))
    return "\n".join([seg(BX0, BY0, BX1, BY0), seg(BX1, BY0, BX1, BY1),
                      seg(BX1, BY1, BX0, BY1), seg(BX0, BY1, BX0, BY0)])


def emit():
    body = ['(kicad_pcb (version 20171130) (generator "gen_pcb")',
            '',
            '  (general (thickness 1.6))',
            '  (paper "A4")',
            '  (layers',
            '    (0 "F.Cu" signal)',
            '    (31 "B.Cu" signal)',
            '    (34 "F.Fab" user)',
            '    (35 "B.Fab" user)',
            '    (36 "F.SilkS" user)',
            '    (37 "B.SilkS" user)',
            '    (39 "F.Mask" user)',
            '    (40 "B.Mask" user)',
            '    (44 "Edge.Cuts" user)',
            '  )',
            '  (setup (pad_to_mask_clearance 0))',
            '  (net 0 "")']
    for name in sorted(NETIDX, key=lambda n: NETIDX[n]):
        body.append('  (net %d "%s")' % (NETIDX[name], name))
    body.append('')
    body.append(edge_connector())
    for comp in COMPONENTS:
        body.append(build_comp(comp))
    body.append(board_outline())
    body.append(')')
    return "\n".join(body) + "\n"


if __name__ == "__main__":
    load_netlist("z280-s100.net")
    out = emit()
    with open("z280-s100.kicad_pcb", "w") as f:
        f.write(out)
    nfp = len(re.findall(r'^\s*\(footprint', out, re.M))
    npad = len(re.findall(r'^\s*\(pad', out, re.M))
    nnet = len(re.findall(r'^\s*\(net ', out, re.M)) - 1  # minus (net 0 "")
    print("wrote z280-s100.kicad_pcb: %d footprints, %d pads, %d nets" % (nfp, npad, nnet))
