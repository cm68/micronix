#!/usr/bin/env python3
"""Generate a KiCad 9 hierarchical schematic for the Z280 -> S-100 CPU card.

Signal-level connectivity follows extra/docs/z280-s100-cpu-card.md. Pin numbers
are correct for the standard parts and the S-100 connector (taken from the
s100z80 reference design in extra/hardware/s100z80); Z280 and ATF1508 pin
numbers are placeholders that must be checked against their datasheets (see the
NOTES.md this script writes).
"""
import os, uuid

OUT = os.path.dirname(os.path.abspath(__file__))
V = "20250114"  # KiCad 9

def uid():
    return uuid.uuid4().hex

E = {"P": "passive", "I": "input", "O": "output", "B": "bidirectional",
     "W": "power_in", "T": "tri_state", "OC": "open_collector"}

# name -> (ref_prefix, value, footprint, [ (num, name, elec), ... ])
PARTS = {}

PARTS["Z280"] = ("U", "Z280 Z-BUS (12 MHz)", "Package_LCC:PLCC-68_SMD",
    [("1","GND","W"), ("2","AD13","B"), ("3","AD14","B"), ("4","AD15","B"),
     ("5","A16","O"), ("6","A17","O"), ("7","A23","O"), ("8","A18","O"),
     ("9","A19","O"), ("10","B/W","O"), ("11","DMASTB0","O"), ("12","R/W","O"),
     ("13","DMASTB1","O"), ("14","ST0","O"), ("15","ST1","O"), ("16","OE","O"),
     ("17","IE","O"), ("18","VCC","W"), ("19","VCC","W"), ("20","CTIO1","B"),
     ("21","ST2","O"), ("22","ST3","O"), ("23","CTIO2","B"), ("24","DS","O"),
     ("25","CTIN2","I"), ("26","INT-C","I"), ("27","AS","O"), ("28","BUSREQ","I"),
     ("29","WAIT","I"), ("30","CTIO0/GREQ","B"), ("31","BUSACK","O"), ("32","CTIN0/GACK","I"),
     ("33","PAUSE","I"), ("34","OPT","I"), ("35","GND","W"), ("36","INT-B/EOP-B","I"),
     ("37","INT-A/EOP-A","I"), ("38","RESET","I"), ("39","NMI","I"), ("40","AD0","B"),
     ("41","CTIN1","I"), ("42","AD1","B"), ("43","AD2","B"), ("44","AD3","B"),
     ("45","A20","O"), ("46","TXD","O"), ("47","CLK","O"), ("48","RXD","I"),
     ("49","XTALO","O"), ("50","XTALI","I"), ("51","GND","W"), ("52","RESERVED","P"),
     ("53","GND","W"), ("54","AD4","B"), ("55","RDY0","I"), ("56","RDY1","I"),
     ("57","AD5","B"), ("58","RDY3","I"), ("59","A21","O"), ("60","AD6","B"),
     ("61","AD7","B"), ("62","AD8","B"), ("63","RDY2","I"), ("64","AD9","B"),
     ("65","AD10","B"), ("66","A22","O"), ("67","AD11","B"), ("68","AD12","B")])

CPLD_A_IO = [
    ("Z_AS","I"),
    ("Z_DS","I"),
    ("Z_RW","I"),
    ("Z_BW","I"),
    ("Z_ST0","I"),
    ("Z_ST1","I"),
    ("Z_ST2","I"),
    ("Z_ST3","I"),
    ("Z_IE","I"),
    ("Z_OE","I"),
    ("Z_BUSACK","I"),
    ("Z_CLK_IN","I"),
    ("Z_RESET","I"),
    ("S100_INT","I"),
    ("S100_NMI","I"),
    ("S100_HOLD","I"),
    ("S100_pRDY","I"),
    ("S100_XRDY","I"),
    ("S100_SIXTN","I"),
    ("SLAVE_ONLY","I"),
    ("LA0","I"),
    ("LA1","I"),
    ("LA2","I"),
    ("SRAM_WIN","I"),
    ("FLASH_WIN","I"),
    ("BANK","I"),
    ("Z_WAIT","O"),
    ("Z_BUSREQ","O"),
    ("Z_INT","O"),
    ("Z_NMI","O"),
    ("CPLD_sMEMR","B"),
    ("CPLD_sWO","B"),
    ("CPLD_sINP","B"),
    ("CPLD_sOUT","B"),
    ("CPLD_sINTA","B"),
    ("CPLD_sHLTA","B"),
    ("CPLD_sXTRQ","B"),
    ("CPLD_pSYNC","O"),
    ("CPLD_pSTVAL","O"),
    ("CPLD_pDBIN","O"),
    ("CPLD_pWR","O"),
    ("CPLD_pHLDA","O"),
    ("MEM_CE0","O"),
    ("MEM_CE1","O"),
    ("MEM_OE","O"),
    ("MEM_WE_L","O"),
    ("MEM_WE_H","O"),
    ("FLASH_CE","O"),
    ("FLASH_OE","O"),
    ("LATCH_LE","O"),
    ("SRAM_A0","O"),
    ("SRAM_A1","O"),
    ("SLAVE","O"),
    ("MASTER_ACTIVE","O"),
    ("MASTER_WRITE","O"),
    ("BYTE_SEL","O"),
    ("XFR16","O"),
    ("TCK","I"),
    ("TMS","I"),
    ("TDI","I"),
    ("TDO","O"),
]
# Verified ATF1508 84-pin PLCC power pins (VCCINT/VCCIO/GND), shared by both CPLDs.
_ATF1508_POWER = [
    ("3","VCCINT","W"), ("43","VCCINT","W"),
    ("13","VCCIO","W"), ("26","VCCIO","W"), ("38","VCCIO","W"),
    ("53","VCCIO","W"), ("66","VCCIO","W"), ("78","VCCIO","W"),
    ("7","GND","W"), ("19","GND","W"), ("32","GND","W"), ("42","GND","W"),
    ("47","GND","W"), ("59","GND","W"), ("72","GND","W"), ("82","GND","W"),
]

# signal -> physical pin, matching z280-s100-control.pld (verified against the
# ATF1508 PLCC-84 pinout; JTAG TDI/TMS/TCK/TDO are dedicated at 14/23/62/71).
_A_PIN = {
    "Z_AS": 1, "Z_DS": 2, "Z_RW": 4, "Z_BW": 5, "Z_ST0": 6, "Z_ST1": 8,
    "Z_ST2": 9, "Z_ST3": 10, "Z_IE": 11, "Z_OE": 12, "Z_BUSACK": 15,
    "Z_CLK_IN": 83, "Z_RESET": 16, "S100_INT": 17, "S100_NMI": 18,
    "S100_HOLD": 20, "S100_pRDY": 21, "S100_XRDY": 22, "S100_SIXTN": 24,
    "SLAVE_ONLY": 25, "LA0": 27, "LA1": 28, "LA2": 29, "SRAM_WIN": 30,
    "FLASH_WIN": 31, "BANK": 33,
    "Z_WAIT": 34, "Z_INT": 35, "Z_NMI": 36, "Z_BUSREQ": 37,
    "CPLD_sMEMR": 39, "CPLD_sWO": 40, "CPLD_sINP": 41, "CPLD_sOUT": 44,
    "CPLD_sINTA": 45, "CPLD_sHLTA": 46, "CPLD_sXTRQ": 48, "CPLD_pSYNC": 49,
    "CPLD_pSTVAL": 50, "CPLD_pDBIN": 51, "CPLD_pWR": 52, "CPLD_pHLDA": 54,
    "MEM_CE0": 55, "MEM_CE1": 56, "MEM_OE": 57, "MEM_WE_L": 58, "MEM_WE_H": 60,
    "FLASH_CE": 61, "FLASH_OE": 63, "LATCH_LE": 64, "SRAM_A0": 65, "SRAM_A1": 67,
    "SLAVE": 68, "MASTER_ACTIVE": 69, "MASTER_WRITE": 70, "BYTE_SEL": 73, "XFR16": 74,
    "TCK": 62, "TMS": 23, "TDI": 14, "TDO": 71,
}
_cpld_a_pins = [(str(_A_PIN[name]), name, elec) for name, elec in CPLD_A_IO]
_cpld_a_pins += _ATF1508_POWER
PARTS["ATF1508"] = ("U", "ATF1508AS (PLCC-84)", "Package_LCC:PLCC-84_SMD", _cpld_a_pins)

CPLD_B_IO = [
    ("SLAVE","I"),
    ("MASTER_ACTIVE","I"),
    ("MASTER_WRITE","I"),
    ("BYTE_SEL","I"),
    ("XFR16","I"),
    ("S100_sMEMR","I"),
    ("S100_sWO","I"),
    ("S100_sXTRQ","I"),
    ("S100_ADSB","I"),
    ("S100_DODSB","I"),
    ("S100_SDSB","I"),
    ("S100_CDSB","I"),
    ("A16","I"),
    ("A17","I"),
    ("A18","I"),
    ("A19","I"),
    ("A20","I"),
    ("A21","I"),
    ("A22","I"),
    ("A23","I"),
    ("AD0","B"),
    ("AD1","B"),
    ("AD2","B"),
    ("AD3","B"),
    ("AD4","B"),
    ("AD5","B"),
    ("AD6","B"),
    ("AD7","B"),
    ("AD8","B"),
    ("AD9","B"),
    ("AD10","B"),
    ("AD11","B"),
    ("AD12","B"),
    ("AD13","B"),
    ("AD14","B"),
    ("AD15","B"),
    ("S100_DO0","B"),
    ("S100_DO1","B"),
    ("S100_DO2","B"),
    ("S100_DO3","B"),
    ("S100_DO4","B"),
    ("S100_DO5","B"),
    ("S100_DO6","B"),
    ("S100_DO7","B"),
    ("S100_DI0","B"),
    ("S100_DI1","B"),
    ("S100_DI2","B"),
    ("S100_DI3","B"),
    ("S100_DI4","B"),
    ("S100_DI5","B"),
    ("S100_DI6","B"),
    ("S100_DI7","B"),
    ("S100_A_OE","O"),
    ("S100_A_DIR","O"),
    ("S100_S_OE","O"),
    ("S100_C_OE","O"),
    ("S100_SC_DIR","O"),
    ("S100_pRDY","B"),
    ("S100_XRDY","B"),
    ("S100_SIXTN","B"),
    ("SRAM_WIN","O"),
    ("FLASH_WIN","O"),
    ("BANK","O"),
    ("TCK","I"),
    ("TMS","I"),
    ("TDI","I"),
    ("TDO","O"),
]
# signal -> physical pin, matching z280-s100-data.pld (verified against the
# ATF1508 PLCC-84 pinout; JTAG TDI/TMS/TCK/TDO are dedicated at 14/23/62/71).
_B_PIN = {
    "SLAVE": 1, "MASTER_ACTIVE": 12, "MASTER_WRITE": 15, "BYTE_SEL": 16, "XFR16": 17,
    "S100_sMEMR": 18, "S100_sWO": 20, "S100_sXTRQ": 21,
    "S100_ADSB": 22, "S100_DODSB": 24, "S100_SDSB": 25, "S100_CDSB": 27,
    "A16": 2, "A17": 4, "A18": 5, "A19": 6, "A20": 8, "A21": 9, "A22": 10, "A23": 11,
    "AD0": 28, "AD1": 29, "AD2": 30, "AD3": 31, "AD4": 33, "AD5": 34, "AD6": 35, "AD7": 36,
    "AD8": 37, "AD9": 39, "AD10": 40, "AD11": 41, "AD12": 44, "AD13": 45, "AD14": 46, "AD15": 48,
    "S100_DO0": 49, "S100_DO1": 50, "S100_DO2": 51, "S100_DO3": 52, "S100_DO4": 54,
    "S100_DO5": 55, "S100_DO6": 56, "S100_DO7": 57,
    "S100_DI0": 58, "S100_DI1": 60, "S100_DI2": 61, "S100_DI3": 63, "S100_DI4": 64,
    "S100_DI5": 65, "S100_DI6": 67, "S100_DI7": 68,
    "S100_A_OE": 74, "S100_A_DIR": 75, "S100_S_OE": 76, "S100_C_OE": 77, "S100_SC_DIR": 79,
    "S100_pRDY": 80, "S100_XRDY": 81, "S100_SIXTN": 83,
    "SRAM_WIN": 69, "FLASH_WIN": 70, "BANK": 73,
    "TCK": 62, "TMS": 23, "TDI": 14, "TDO": 71,
}
_cpld_b_pins = [(str(_B_PIN[name]), name, elec) for name, elec in CPLD_B_IO]
_cpld_b_pins += _ATF1508_POWER
PARTS["ATF1508B"] = ("U", "ATF1508AS (PLCC-84)", "Package_LCC:PLCC-84_SMD", _cpld_b_pins)

PARTS["IS61C5128AS"] = ("U", "IS61C5128AS-25 (512Kx8)", "Package_DIP:DIP-32",
    [("1","A14","I"),("2","A12","I"),("3","A7","I"),("4","A6","I"),
     ("5","A5","I"),("6","A4","I"),("7","A3","I"),("8","A2","I"),
     ("9","A1","I"),("10","A0","I"),("11","DQ0","B"),("12","DQ1","B"),
     ("13","DQ2","B"),("14","GND","W"),("15","DQ3","B"),("16","DQ4","B"),
     ("17","DQ5","B"),("18","DQ6","B"),("19","DQ7","B"),("20","CE","I"),
     ("21","A10","I"),("22","OE","I"),("23","A11","I"),("24","A9","I"),
     ("25","A8","I"),("26","A13","I"),("27","WE","I"),("28","VCC","W"),
     ("29","A18","I"),("30","A17","I"),("31","A16","I"),("32","A15","I")])

# SST27SF020 (2 Mbit, 256Kx8) in a 32-pin DIP socket. JEDEC 27C020 pinout. The
# upper 6 pins (1/2/3/30/31/32) route through JP jumpers for density flexibility.
PARTS["SST27SF020"] = ("U", "SST27SF020 (256Kx8)", "Package_DIP:DIP-32",
    [("1","VPP","P"),("2","A16","I"),("3","A15","I"),("4","A12","I"),
     ("5","A7","I"),("6","A6","I"),("7","A5","I"),("8","A4","I"),
     ("9","A3","I"),("10","A2","I"),("11","A1","I"),("12","A0","I"),
     ("13","DQ0","B"),("14","DQ1","B"),("15","DQ2","B"),("16","GND","W"),
     ("17","DQ3","B"),("18","DQ4","B"),("19","DQ5","B"),("20","DQ6","B"),
     ("21","DQ7","B"),("22","CE","I"),("23","A10","I"),("24","OE","I"),
     ("25","A11","I"),("26","A9","I"),("27","A8","I"),("28","A13","I"),
     ("29","A14","I"),("30","A17","I"),("31","PGM","I"),("32","VCC","W")])

PARTS["JP"] = ("J", "Jumper (2-pin header)", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
    [("1","A","P"),("2","B","P")])

PARTS["74HC573"] = ("U", "74HC573", "Package_DIP:DIP-20",
    [("1","OE","I"),("2","D0","I"),("3","D1","I"),("4","D2","I"),
     ("5","D3","I"),("6","D4","I"),("7","D5","I"),("8","D6","I"),
     ("9","D7","I"),("10","GND","W"),("11","LE","I"),("12","Q0","T"),
     ("13","Q1","T"),("14","Q2","T"),("15","Q3","T"),("16","Q4","T"),
     ("17","Q5","T"),("18","Q6","T"),("19","Q7","T"),("20","VCC","W")])

PARTS["74HCT245"] = ("U", "74HCT245", "Package_DIP:DIP-20",
    [("1","DIR","I"),("2","A0","B"),("3","A1","B"),("4","A2","B"),
     ("5","A3","B"),("6","A4","B"),("7","A5","B"),("8","A6","B"),
     ("9","A7","B"),("10","GND","W"),("11","B0","B"),("12","B1","B"),
     ("13","B2","B"),("14","B3","B"),("15","B4","B"),("16","B5","B"),
     ("17","B6","B"),("18","B7","B"),("19","OE","I"),("20","VCC","W")])

PARTS["MAX232"] = ("U", "MAX232", "Package_DIP:DIP-16",
    [("1","C1+","P"),("2","V+","P"),("3","C1-","P"),("4","C2+","P"),
     ("5","C2-","P"),("6","V-","P"),("7","T2OUT","O"),("8","R2IN","I"),
     ("9","R2OUT","O"),("10","T2IN","I"),("11","T1IN","I"),
     ("12","R1OUT","O"),("13","R1IN","I"),("14","T1OUT","O"),
     ("15","GND","W"),("16","VCC","W")])

PARTS["DS1813"] = ("U", "DS1813", "Package_TO_SOT:TO-92",
    [("1","GND","W"),("2","RST","OC"),("3","VCC","W")])

PARTS["LM7805"] = ("U", "LM7805", "Package_TO_SOT:TO-220-3",
    [("1","IN","I"),("2","GND","W"),("3","OUT","O")])

PARTS["Crystal"] = ("Y", "24 MHz", "Crystal:Crystal_HC49",
    [("1","X1","P"),("2","X2","P")])

PARTS["JTAG"] = ("J", "JTAG header (1x6)", "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical",
    [("1","TCK","B"),("2","TMS","B"),("3","TDI","B"),("4","TDO","B"),
     ("5","GND","W"),("6","VCC","W")])

PARTS["R"] = ("R", "1k", "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal",
    [("1","1","P"),("2","2","P")])

S100 = [("1","+8V","W"),("2","+16V","W"),("3","XRDY","B"),("4","VI0","P"),
    ("5","VI1","P"),("6","VI2","P"),("7","VI3","P"),("8","VI4","P"),
    ("9","VI5","P"),("10","VI6","P"),("11","VI7","P"),("12","NMI","B"),
    ("13","PWRFAIL","P"),("14","DMA3","P"),("15","A18","B"),("16","A16","B"),
    ("17","A17","B"),("18","SDSB","B"),("19","CDSB","B"),("20","GND","W"),
    ("21","NC","P"),("22","ADSB","B"),("23","DODSB","B"),("24","PHI","P"),
    ("25","pSTVAL","B"),("26","pHLDA","B"),("27","NC","P"),("28","NC","P"),
    ("29","A5","B"),("30","A4","B"),("31","A3","B"),("32","A15","B"),
    ("33","A12","B"),("34","A9","B"),("35","DO1","B"),("36","DO0","B"),
    ("37","A10","B"),("38","DO4","B"),("39","DO5","B"),("40","DO6","B"),
    ("41","DI2","B"),("42","DI3","B"),("43","DI7","B"),("44","sM1","B"),
    ("45","sOUT","B"),("46","sINP","B"),("47","sMEMR","B"),("48","sHLTA","B"),
    ("49","CLOCK","P"),("50","GND","W"),
    ("51","+8V","W"),("52","-16V","W"),("53","GND","W"),("54","SLAVE_CLR","P"),
    ("55","DMA0","P"),("56","DMA1","P"),("57","DMA2","P"),("58","sXTRQ","B"),
    ("59","A19","B"),("60","SIXTN","B"),("61","A20","B"),("62","A21","B"),
    ("63","A22","B"),("64","A23","B"),("65","NC","P"),("66","NC","P"),
    ("67","PHANTOM","P"),("68","MWRT","P"),("69","NC","P"),("70","GND","W"),
    ("71","NC","P"),("72","pRDY","B"),("73","INT","B"),("74","HOLD","B"),
    ("75","RESET","B"),("76","pSYNC","B"),("77","pWR","B"),("78","pDBIN","B"),
    ("79","A0","B"),("80","A1","B"),("81","A2","B"),("82","A6","B"),
    ("83","A7","B"),("84","A8","B"),("85","A13","B"),("86","A14","B"),
    ("87","A11","B"),("88","DO2","B"),("89","DO3","B"),("90","DO7","B"),
    ("91","DI4","B"),("92","DI5","B"),("93","DI6","B"),("94","DI1","B"),
    ("95","DI0","B"),("96","sINTA","B"),("97","sWO","B"),("98","ERROR","P"),
    ("99","POC","P"),("100","GND","W")]
PARTS["S100_100"] = ("J", "S-100 edge connector (100-pin)", "Connector_Edge", S100)

# ----------------------------------------------------------------------------
def body_metrics(name):
    pins = PARTS[name][3]
    n = len(pins); half = (n + 1) // 2
    return half, 12.7, (max(half, n - half) + 1) * 2.54

def emit_pin(num, name, elec, x, y, angle=0):
    return ('    (pin %s line (at %.2f %.2f %d) (length 2.54)\n'
            '      (name "%s" (effects (font (size 1.27 1.27))))\n'
            '      (number "%s" (effects (font (size 1.27 1.27))))\n    )'
            % (E[elec], x, y, angle, name, num))

def emit_symbol(name, full_name=None):
    if full_name is None:
        full_name = name
    ref, value, fp, pins = PARTS[name]
    n = len(pins); half = (n + 1) // 2
    body_w = 20.32; max_side = max(half, n - half)
    L = []
    L.append('  (symbol "%s"' % full_name)
    L.append('    (pin_names (offset 1.016))')
    L.append('    (exclude_from_sim no) (in_bom yes) (on_board yes)')
    L.append('    (property "Reference" "%s" (at 0 %.2f 0) (effects (font (size 1.27 1.27))))' % (ref, 5.08))
    L.append('    (property "Value" "%s" (at 0 %.2f 0) (effects (font (size 1.27 1.27))))' % (value, -max_side*2.54 - 2.54))
    L.append('    (property "Footprint" "%s" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))' % fp)
    L.append('    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    L.append('    (symbol "%s_0_1"' % name)
    L.append('      (rectangle (start -%.2f -%.2f) (end %.2f %.2f) (stroke (width 0.254) (type default)) (fill (type background)))' % (body_w/2, max_side*2.54, body_w/2, 2.54))
    L.append('    )')
    L.append('    (symbol "%s_1_1"' % name)
    for i, (num, nm, e) in enumerate(pins):
        y = -(i % half) * 2.54
        if i < half:
            L.append(emit_pin(num, nm, e, -body_w/2 - 2.54, y, 0))
        else:
            L.append(emit_pin(num, nm, e, body_w/2 + 2.54, y, 180))
    L.append('    )')
    L.append('  )')
    return '\n'.join(L)

def pin_abs(name, num, sym_x, sym_y):
    ref, value, fp, pins = PARTS[name]
    n = len(pins); half = (n + 1) // 2
    body_w = 20.32
    for i, (pn, nm, e) in enumerate(pins):
        if str(pn) == str(num):
            y = -(i % half) * 2.54
            if i < half:
                x = -body_w/2 - 2.54; side = -1
            else:
                x = body_w/2 + 2.54; side = 1
            return (sym_x + x, sym_y - y, side)  # symbol Y is negated in schematic space
    raise KeyError((name, num))

def emit_symbol_lib():
    body = ['(kicad_symbol_lib', '  (version %s)' % V,
            '  (generator "gen_kicad")', '  (generator_version "9.0")']
    for pname in PARTS:
        body.append(emit_symbol(pname))
    body.append(')')
    return '\n'.join(body) + '\n'

def emit_label(net, x, y, kind="global", side=1):
    # Left-side pins get a horizontally-mirrored label: text extends left.
    justify = "right" if side < 0 else "left"
    if kind == "global":
        return ('  (global_label "%s" (shape input) (at %.2f %.2f 0)\n'
                '    (effects (font (size 1.27 1.27)) (justify %s))\n'
                '    (uuid "%s")\n'
                '    (property "Intersheetrefs" "${INTERSHEET_REFS}" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n  )'
                % (net, x, y, justify, uid()))
    return ('  (label "%s" (at %.2f %.2f 0)\n'
            '    (effects (font (size 1.27 1.27)) (justify %s))\n'
            '    (uuid "%s")\n  )' % (net, x, y, justify, uid()))

def emit_symbol_instance(name, ref, sym_x, sym_y, nets):
    _ref, value, fp, pins = PARTS[name]
    lines = ['  (symbol (lib_id "z280s100:%s")' % name]
    lines.append('    (at %.2f %.2f 0)' % (sym_x, sym_y))
    lines.append('    (unit 1) (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp no) (fields_autoplaced yes)')
    lines.append('    (uuid "%s")' % uid())
    lines.append('    (property "Reference" "%s" (at %.2f %.2f 0) (effects (font (size 1.27 1.27))))' % (ref, sym_x, sym_y - 2.54))
    lines.append('    (property "Value" "%s" (at %.2f %.2f 0) (effects (font (size 1.27 1.27))))' % (value, sym_x, sym_y + 2.54))
    lines.append('    (property "Footprint" "%s" (at %.2f %.2f 0) (effects (font (size 1.27 1.27)) hide))' % (fp, sym_x, sym_y))
    lines.append('    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    for num, nm, e in pins:
        lines.append('    (pin "%s" (uuid "%s"))' % (num, uid()))
    lines.append('    (instances (project "z280-s100" (path "%s" (reference "%s") (unit 1))))' % (OUT, ref))
    lines.append('  )')
    for num, net in nets.items():
        x, y, side = pin_abs(name, num, sym_x, sym_y)
        ex = x + side * 5.08
        lines.append('  (wire (pts (xy %.2f %.2f) (xy %.2f %.2f)) (stroke (width 0) (type default)) (uuid "%s"))' % (x, y, ex, y, uid()))
        lines.append(emit_label(net, ex, y, side=side))
    return '\n'.join(lines)

def lib_symbols_section():
    body = ['  (lib_symbols']
    for name in PARTS:
        body.append(emit_symbol(name, "z280s100:" + name))
    body.append('  )')
    return '\n'.join(body)

def emit_sheet_file(title, instances, paper="A1"):
    body = ['(kicad_sch', '  (version %s)' % V, '  (generator "gen_kicad")',
            '  (generator_version "9.0")', '  (uuid "%s")' % uid(),
            '  (paper "%s")' % paper,
            '  (title_block (title "%s") (date "2026-08-20"))' % title]
    body.append(lib_symbols_section())
    body.extend(instances)
    body.append(')')
    return '\n'.join(body) + '\n'

# ----------------------------------------------------------------------------
def sram_nets(data_nets, we, ce):
    d = {"1":"LA15","2":"LA13","3":"LA8","4":"LA7","5":"LA6","6":"LA5",
         "7":"LA4","8":"LA3","9":"SRAM_A1","10":"SRAM_A0","11":data_nets[0],
         "12":data_nets[1],"13":data_nets[2],"14":"GND","15":data_nets[3],
         "16":data_nets[4],"17":data_nets[5],"18":data_nets[6],"19":data_nets[7],
         "20":ce,"21":"LA11","22":"MEM_OE","23":"LA12","24":"LA10",
         "25":"LA9","26":"LA14","27":we,"28":"+5V","29":"A19","30":"A18",
         "31":"A17","32":"A16"}
    return d

def flash_nets(data_nets):
    # SST27SF020 (256Kx8), JEDEC 27C020. Word-addressed: flash A_n = byte A_{n+1}.
    # Upper 6 pins (VPP/A16/A15/A17/PGM#/VDD) route through JP jumpers so a
    # smaller EPROM can be fitted by moving the address jumpers.
    d = {"12":"LA1","11":"LA2","10":"LA3","9":"LA4","8":"LA5","7":"LA6",
         "6":"LA7","5":"LA8","27":"LA9","26":"LA10","23":"LA11","25":"LA12",
         "4":"LA13","28":"LA14","29":"LA15",
         "3":"FLASH_A15","2":"FLASH_A16","30":"FLASH_A17",
         "13":data_nets[0],"14":data_nets[1],"15":data_nets[2],"17":data_nets[3],
         "18":data_nets[4],"19":data_nets[5],"20":data_nets[6],"21":data_nets[7],
         "22":"FLASH_CE","24":"FLASH_OE",
         "1":"FLASH_VPP","31":"FLASH_PGM","32":"FLASH_VDD",
         "16":"GND"}
    return d

def _cpld_nets(pins, tag):
    nets = {}
    for num, nm, e in pins:
        if nm in ("VCC", "VCCINT", "VCCIO"):
            nets[num] = "+5V"
        elif nm == "GND":
            nets[num] = "GND"
        elif nm == "Z_CLK_IN":
            nets[num] = "Z_CLK"                # Z280 CLK output feeds the CPLD clock
        elif nm in ("TCK","TMS"):
            nets[num] = "JTAG_" + nm            # shared across both CPLDs
        elif nm == "TDI":
            nets[num] = "JTAG_TDI" if tag == "A" else "JTAG_CHAIN"
        elif nm == "TDO":
            nets[num] = "JTAG_CHAIN" if tag == "A" else "JTAG_TDO"
        else:
            nets[num] = nm
    return nets

def cpld_a_nets():
    return _cpld_nets(_cpld_a_pins, "A")

def cpld_b_nets():
    return _cpld_nets(_cpld_b_pins, "B")

def s100_nets():
    nets = {}
    for num, nm, e in S100:
        if nm in ("+8V","+16V","-16V","GND"):
            nets[num] = nm
        elif nm == "NC":
            continue
        else:
            nets[num] = "S100_" + nm
    return nets

def buf_nets(alist, blist, dirn, oen):
    d = {"1":dirn, "10":"GND", "19":oen, "20":"+5V"}
    for i, (a, b) in enumerate(zip(alist, blist)):
        d[str(2+i)] = a
        d[str(11+i)] = b
    return d

def single_sheet():
    inst = []
    # CPU + address latches
    z = {"40":"AD0","42":"AD1","43":"AD2","44":"AD3","54":"AD4","57":"AD5",
         "60":"AD6","61":"AD7","62":"AD8","64":"AD9","65":"AD10","67":"AD11",
         "68":"AD12","2":"AD13","3":"AD14","4":"AD15",
         "5":"A16","6":"A17","8":"A18","9":"A19","45":"A20","59":"A21","66":"A22","7":"A23",
         "27":"Z_AS","24":"Z_DS","12":"Z_RW","10":"Z_BW",
         "14":"Z_ST0","15":"Z_ST1","21":"Z_ST2","22":"Z_ST3",
         "17":"Z_IE","16":"Z_OE","29":"Z_WAIT","28":"Z_BUSREQ","31":"Z_BUSACK",
         "37":"Z_INT","39":"Z_NMI","38":"Z_RESET",
         "50":"XTALI","49":"XTALO","47":"Z_CLK",
         "34":"+5V","46":"Z_TXD","48":"Z_RXD",
         "18":"+5V","19":"+5V",
         "1":"GND","35":"GND","51":"GND","53":"GND"}
    inst.append(emit_symbol_instance("Z280", "U1", 38.1, 25.4, z))
    latch_lo = {"1":"GND","2":"AD0","3":"AD1","4":"AD2","5":"AD3","6":"AD4",
                "7":"AD5","8":"AD6","9":"AD7","10":"GND","11":"LATCH_LE",
                "12":"LA0","13":"LA1","14":"LA2","15":"LA3","16":"LA4",
                "17":"LA5","18":"LA6","19":"LA7","20":"+5V"}
    inst.append(emit_symbol_instance("74HC573", "U3", 38.1, 127, latch_lo))
    latch_hi = {"1":"GND","2":"AD8","3":"AD9","4":"AD10","5":"AD11","6":"AD12",
                "7":"AD13","8":"AD14","9":"AD15","10":"GND","11":"LATCH_LE",
                "12":"LA8","13":"LA9","14":"LA10","15":"LA11","16":"LA12",
                "17":"LA13","18":"LA14","19":"LA15","20":"+5V"}
    inst.append(emit_symbol_instance("74HC573", "U4", 38.1, 165.1, latch_hi))
    # CPLD
    inst.append(emit_symbol_instance("ATF1508", "U2", 101.6, 25.4, cpld_a_nets()))
    inst.append(emit_symbol_instance("ATF1508B", "U24", 101.6, 127.0, cpld_b_nets()))
    # Memory: U9/U11 = even/LO on AD8-15, U10/U12 = odd/HI on AD0-7
    inst.append(emit_symbol_instance("IS61C5128AS", "U9", 165.1, 25.4,
                 sram_nets([f"AD{i}" for i in range(8,16)], "MEM_WE_L", "MEM_CE0")))
    inst.append(emit_symbol_instance("IS61C5128AS", "U10", 165.1, 76.2,
                 sram_nets([f"AD{i}" for i in range(8)], "MEM_WE_H", "MEM_CE0")))
    inst.append(emit_symbol_instance("IS61C5128AS", "U22", 165.1, 127.0,
                 sram_nets([f"AD{i}" for i in range(8,16)], "MEM_WE_L", "MEM_CE1")))
    inst.append(emit_symbol_instance("IS61C5128AS", "U23", 165.1, 177.8,
                 sram_nets([f"AD{i}" for i in range(8)], "MEM_WE_H", "MEM_CE1")))
    inst.append(emit_symbol_instance("SST27SF020", "U11", 228.6, 25.4,
                 flash_nets([f"AD{i}" for i in range(8,16)])))
    inst.append(emit_symbol_instance("SST27SF020", "U12", 228.6, 76.2,
                 flash_nets([f"AD{i}" for i in range(8)])))
    # Flash upper-6-pin jumper area (shared by U11/U12). Default = 27SF020;
    # for a smaller EPROM move/remove the A15/A16/A17 jumpers. A 28-pin part
    # (JEDEC +2 offset) lands its VDD (pin 28) on socket pin 30 -> move J6 to +5V.
    # VPP/PGM#/VDD tie +5V.
    for i, (jp, a, b) in enumerate([
            ("J3", "FLASH_VPP", "+5V"),
            ("J4", "FLASH_A15", "A16"),
            ("J5", "FLASH_A16", "A17"),
            ("J6", "FLASH_A17", "A18"),
            ("J7", "FLASH_PGM", "+5V"),
            ("J8", "FLASH_VDD", "+5V")]):
        inst.append(emit_symbol_instance("JP", jp, 292.1, 25.4 + i * 5.08, {"1": a, "2": b}))
    # S-100 connector (the byte-steered data path lives inside CPLD B / U24)
    inst.append(emit_symbol_instance("S100_100", "J1", 355.6, 25.4, s100_nets()))
    # Power / clock / reset / console
    inst.append(emit_symbol_instance("LM7805", "U15", 228.6, 139.7,
                 {"1":"+8V","2":"GND","3":"+5V"}))
    inst.append(emit_symbol_instance("Crystal", "Y1", 228.6, 127.0,
                 {"1":"XTALI","2":"XTALO"}))
    inst.append(emit_symbol_instance("DS1813", "U14", 228.6, 165.1,
                 {"1":"GND","2":"Z_RESET","3":"+5V"}))
    inst.append(emit_symbol_instance("MAX232", "U13", 292.1, 177.8,
                 {"1":"MAX_C1P","2":"MAX_VP","3":"MAX_C1M","4":"MAX_C2P",
                  "5":"MAX_C2M","6":"MAX_VM","7":"NC","8":"NC","9":"NC",
                  "10":"NC","11":"Z_TXD","12":"NC","13":"Z_RXD",
                  "14":"MAX_RS232_TX","15":"GND","16":"+5V"}))
    # S-100 drive buffers (74HCT245): address + status + control
    inst.append(emit_symbol_instance("74HCT245", "U16", 406.4, 25.4,
                 buf_nets(["BYTE_SEL"] + [f"LA{i}" for i in range(1,8)], [f"S100_A{i}" for i in range(8)], "S100_A_DIR", "S100_A_OE")))
    inst.append(emit_symbol_instance("74HCT245", "U17", 406.4, 63.5,
                 buf_nets([f"LA{i}" for i in range(8,16)], [f"S100_A{i}" for i in range(8,16)], "S100_A_DIR", "S100_A_OE")))
    inst.append(emit_symbol_instance("74HCT245", "U18", 406.4, 101.6,
                 buf_nets([f"A{i}" for i in range(16,24)], [f"S100_A{i}" for i in range(16,24)], "S100_A_DIR", "S100_A_OE")))
    inst.append(emit_symbol_instance("74HCT245", "U19", 406.4, 139.7,
                 buf_nets(["CPLD_sMEMR","CPLD_sWO","CPLD_sINP","CPLD_sOUT","CPLD_sINTA","CPLD_sHLTA","CPLD_sXTRQ"],
                          ["S100_sMEMR","S100_sWO","S100_sINP","S100_sOUT","S100_sINTA","S100_sHLTA","S100_sXTRQ"],
                          "S100_SC_DIR", "S100_S_OE")))
    inst.append(emit_symbol_instance("74HCT245", "U20", 406.4, 177.8,
                 buf_nets(["CPLD_pSYNC","CPLD_pSTVAL","CPLD_pDBIN","CPLD_pWR"],
                          ["S100_pSYNC","S100_pSTVAL","S100_pDBIN","S100_pWR"],
                          "S100_SC_DIR", "S100_C_OE")))
    # pHLDA is the permanent master's *exclusive* output, asserted while a TMA
    # holds the bus -- opposite direction from pSYNC/pDBIN/pWR -- so it gets its
    # own always-on driver (74HCT245 strapped A->B). Unused A inputs tied low.
    inst.append(emit_symbol_instance("74HCT245", "U21", 406.4, 215.9,
                 {"1":"+5V","10":"GND","19":"GND","20":"+5V",
                  "2":"CPLD_pHLDA","11":"S100_pHLDA",
                  "3":"GND","4":"GND","5":"GND","6":"GND","7":"GND","8":"GND","9":"GND"}))
    # Local pull-ups for the open-drain ready lines (the backplane also pulls
    # these up; 1k in parallel just strengthens it and keeps the card sane solo).
    inst.append(emit_symbol_instance("R", "R1", 368.3, 241.3, {"1":"+5V","2":"S100_pRDY"}))
    inst.append(emit_symbol_instance("R", "R2", 368.3, 254.0, {"1":"+5V","2":"S100_XRDY"}))
    # SLAVE_ONLY strap: pull down (default = master); jumper to +5V for permanent slave.
    inst.append(emit_symbol_instance("R", "R3", 368.3, 266.7, {"1":"GND","2":"SLAVE_ONLY"}))
    # SIXTN is open-collector (wired-OR); pull up like the ready lines.
    inst.append(emit_symbol_instance("R", "R4", 368.3, 279.4, {"1":"+5V","2":"S100_SIXTN"}))
    # JTAG programming header: TCK/TMS parallel, TDI->A->B->TDO chained
    inst.append(emit_symbol_instance("JTAG", "J2", 406.4, 292.1,
                 {"1":"JTAG_TCK","2":"JTAG_TMS","3":"JTAG_TDI","4":"JTAG_TDO","5":"GND","6":"+5V"}))
    return emit_sheet_file("Z280 S-100 CPU card", inst)

def emit_pro():
    return ('(kicad_project (version 1) (generator "gen_kicad") (generator_version "9.0")\n'
            '  (uuid "%s")\n)\n' % uid())

NOTES = """# Schematic notes — verify before PCB

Signal connectivity follows `extra/docs/z280-s100-cpu-card.md`. These items need
verification against datasheets before this schematic is PCB-ready.

## Pin numbers to verify
- **Z280 (U1)**: 68-pin PLCC pinout verified — Z80 Family Data Book Fig. 2b
  (Z-BUS, OPT=1), transcribed from `extra/docs/z280-pins.tif`. Power = 2× VCC
  (18/19) + 4× GND (1/35/51/53); shared pins GREQ=CTIO0 (30), GACK=CTIN0 (32),
  EOP-A=INT-A (37), EOP-B=INT-B (36).
- **ATF1508 (U2/U24)**: pin numbers are now verified against the PLCC-84 pinout
  and match `z280-s100-control.pld` / `z280-s100-data.pld` — JTAG TDI/TMS/TCK/TDO
  at 14/23/62/71 (dedicated), VCCINT 3/43 + VCCIO 13/26/38/53/66/78, GND
  7/19/32/42/47/59/72/82, Z_CLK_IN on GCK1 = 83. Control uses 57 of 64 I/O, data
  63 of 64 (pin 84 = OE1 spare).
- **SST27SF020 flash (U11/U12)**: 32-pin DIP **socket**, JEDEC 27C020 layout
  (VPP=1, A16=2, A15=3, A17=30, PGM#=31, VDD=32; A0=12 … A14=29; DQ0=13 … DQ7=21;
  CE#=22, OE#=24, VSS=16). Word-addressed (flash A_n = byte A_{n+1}). The upper
  6 pins route through JP jumpers (J3–J8) so a smaller JEDEC EPROM can be fitted
  by moving the A15/A16/A17 jumpers; VPP/PGM#/VDD tie to +5V. A 28-pin part
  (27SF512, JEDEC +2 offset) puts its VDD on socket pin 30 — move J6 from A18 to
  +5V and leave J5/A16 unpopulated.

## Corrected S-100 pinout
Taken from `extra/hardware/s100z80/s100_Z80 V2-cache.lib` (S100_MALE). Key pins:
sMEMR=47, sWO=97, sINP=46, sOUT=45, sM1=44, sINTA=96, sHLTA=48, sXTRQ=58,
SIXTN=60, pSYNC=76, pDBIN=78, pWR=77, pSTVAL=25, pHLDA=26, pRDY=72, XRDY=3,
HOLD=74, RESET=75, INT=73, NMI=12, ADSB=22, DODSB=23, SDSB=18, CDSB=19,
DO0=36/DO1=35/DO2=88/DO3=89/DO4=38/DO5=39/DO6=40/DO7=90,
DI0=95/DI1=94/DI2=41/DI3=42/DI4=91/DI5=92/DI6=93/DI7=43.

## Wiring gaps (currently labeled but not fully connected)
- Interrupts: S100_INT / S100_NMI route through the CPLD to Z_INT / Z_NMI.
- Reset OR: DS1813 reset and S100_RESET must be OR'd before Z_RESET (diode-OR or
  CPLD input).
- LATCH_LE = ~AS (the CPLD inverts Z_AS to drive the 74HC573 latch enable).
- S-100 address drivers (A0-23, DO, status) need tri-state buffers (e.g. 74HCT244)
  gated by S100_A_OE / ADSB / DODSB / SDSB / CDSB — not yet placed.
- Power flags / ERC cleanup and decoupling caps not yet placed.
"""

def main():
    os.makedirs(OUT, exist_ok=True)
    files = {
        "z280-s100.kicad_sym": emit_symbol_lib(),
        "z280-s100.kicad_pro": emit_pro(),
        "z280-s100.kicad_sch": single_sheet(),
        "sym-lib-table": ('(sym_lib_table\n'
                          '  (version 7)\n'
                          '  (lib (name "z280s100")(type "KiCad")'
                          '(uri "${KIPRJMOD}/z280-s100.kicad_sym")'
                          '(options "")(descr "Z280 S-100 CPU card"))\n'
                          ')\n'),
        "NOTES.md": NOTES,
    }
    for fn, content in files.items():
        with open(os.path.join(OUT, fn), "w") as f:
            f.write(content)
        print("wrote", fn, len(content), "bytes")

if __name__ == "__main__":
    main()
