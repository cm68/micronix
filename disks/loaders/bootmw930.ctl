; names for BOOTMW, the CP/M program that boots Micronix from the
; HD/DMA controller, for src/tools/disas
;
;	disas -f bootmw930.ctl -a 0x100 bootmw930.bin
;
; These names are not guesses.  BOOTMW.ASM is in the tree beside the
; binary and assembles to it byte for byte, so every label below is the
; author's own, taken from the assembler's listing.  The disassembly is
; only here to be read against what runs in the simulator.
;
; The first kilobyte is not code.  "buffer: jmp start" then "ds 1021"
; reserves the place the boot loader gets read into, so the program
; proper starts at 0500 and the buffer is what it jumps to at the end.
;
start 0100
bytes 0103 1021
code 0500

define buffer 0100
define start 0500
define again 0502
define ready 0516
define retry 0526
define errpr 0535
define bfail 053d
define nosec 0564
define mwldrv 0586
define zret 05a4
define mwdrv 05a6
define mwsel 05a9
define mwstat 05ae
define mwhome 05b3
define mwseek 05d3
define mwskip0 05ef
define mwsout 0601
define mwskip 0604
define mwdma 061a
define mwsec 0620
define mwdspt 062f
define mwdsptx 0631
define mwreset 0638
define mwread 065e
define mwprep 0660
define mwpreps 067e
define mwissue 0698
define mwiloop 06a2
define mwptr 06b3
define mwneghl 06bf
define mwhlmde 06c7
define mwhlcde 06ce
define mwtab 06d4
define mwcurl 06dc
define mwdrive 06dd
define mwhead 06de
define mwsectr 06df
define dmasel0 06e0
define dmastep 06e1
define dmasel1 06e3
define dmadma 06e4
define dmarg0 06e7
define dmarg1 06e8
define dmarg2 06e9
define dmarg3 06ea
define dmaop 06eb
define dmastat 06ec
define dmalnk 06ed
