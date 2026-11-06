#!/usr/bin/env python3
"""Convert a 2 KB monitor binary to a Quartus MIF for rom2k."""
import sys

def main():
    if len(sys.argv) != 3:
        sys.exit("usage: bin2mif.py mon500.bin mon500.mif")
    src, dst = sys.argv[1], sys.argv[2]
    with open(src, "rb") as f:
        data = f.read()
    if len(data) != 2048:
        sys.exit(f"{src}: expected 2048 bytes, got {len(data)}")
    with open(dst, "w") as f:
        f.write("WIDTH=8;\n")
        f.write("DEPTH=2048;\n")
        f.write("ADDRESS_RADIX=HEX;\n")
        f.write("DATA_RADIX=HEX;\n")
        f.write("CONTENT BEGIN\n")
        for i, b in enumerate(data):
            f.write(f"{i:04X} : {b:02X};\n")
        f.write("END;\n")

if __name__ == "__main__":
    main()
