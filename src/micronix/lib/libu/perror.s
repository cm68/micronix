;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

perror.o:
    0      c.r4: 0000 08 global 
    1     c.ret: 0000 08 global 
    2     c.ent: 0000 08 global 
    3    c.rets: 0000 08 global 
    4    c.ents: 0000 08 global 
    5    _write: 0000 08 global 
    6    _errno: 0000 08 global 
    7   _perror: 0234 0d global defined code 
0000: nop                            ; 00             .    
0001: ld b,d                         ; 42             b    
0002: ld (hl),d                      ; 72             r    
0003: ld l,a                         ; 6f             o    
0004: ld l,e                         ; 6b             k    
0005: ld h,l                         ; 65             e    
0006: ld l,(hl)                      ; 6e             n    
0007: jr nz,.+112                    ; 20 70          .p   
0009: ld l,c                         ; 69             i    
000a: ld (hl),b                      ; 70             p    
000b: ld h,l                         ; 65             e    
000c: nop                            ; 00             .    
000d: ld d,h                         ; 54             t    
000e: ld l,a                         ; 6f             o    
000f: ld l,a                         ; 6f             o    
0010: jr nz,.+109                    ; 20 6d          .m   
0012: ld h,c                         ; 61             a    
0013: ld l,(hl)                      ; 6e             n    
0014: ld a,c                         ; 79             y    
0015: jr nz,.+108                    ; 20 6c          .l   
0017: ld l,c                         ; 69             i    
0018: ld l,(hl)                      ; 6e             n    
0019: ld l,e                         ; 6b             k    
001a: ld (hl),e                      ; 73             s    
001b: nop                            ; 00             .    
001c: ld d,d                         ; 52             r    
001d: ld h,l                         ; 65             e    
001e: ld h,c                         ; 61             a    
001f: ld h,h                         ; 64             d    
0020: dec l                          ; 2d             -    
0021: ld l,a                         ; 6f             o    
0022: ld l,(hl)                      ; 6e             n    
0023: ld l,h                         ; 6c             l    
0024: ld a,c                         ; 79             y    
0025: jr nz,.+102                    ; 20 66          .f   
0027: ld l,c                         ; 69             i    
0028: ld l,h                         ; 6c             l    
0029: ld h,l                         ; 65             e    
002a: jr nz,.+115                    ; 20 73          .s   
002c: ld a,c                         ; 79             y    
002d: ld (hl),e                      ; 73             s    
002e: ld (hl),h                      ; 74             t    
002f: ld h,l                         ; 65             e    
0030: ld l,l                         ; 6d             m    
0031: nop                            ; 00             .    
0032: ld c,c                         ; 49             i    
0033: ld l,h                         ; 6c             l    
0034: ld l,h                         ; 6c             l    
0035: ld h,l                         ; 65             e    
0036: ld h,a                         ; 67             g    
0037: ld h,c                         ; 61             a    
0038: ld l,h                         ; 6c             l    
0039: jr nz,.+115                    ; 20 73          .s   
003b: ld h,l                         ; 65             e    
003c: ld h,l                         ; 65             e    
003d: ld l,e                         ; 6b             k    
003e: nop                            ; 00             .    
003f: ld c,(hl)                      ; 4e             n    
0040: ld l,a                         ; 6f             o    
0041: jr nz,.+115                    ; 20 73          .s   
0043: ld (hl),b                      ; 70             p    
0044: ld h,c                         ; 61             a    
0045: ld h,e                         ; 63             c    
0046: ld h,l                         ; 65             e    
0047: jr nz,.+108                    ; 20 6c          .l   
0049: ld h,l                         ; 65             e    
004a: ld h,(hl)                      ; 66             f    
004b: ld (hl),h                      ; 74             t    
004c: jr nz,.+111                    ; 20 6f          .o   
004e: ld l,(hl)                      ; 6e             n    
004f: jr nz,.+100                    ; 20 64          .d   
0051: ld h,l                         ; 65             e    
0052: halt                           ; 76             v    
0053: ld l,c                         ; 69             i    
0054: ld h,e                         ; 63             c    
0055: ld h,l                         ; 65             e    
0056: nop                            ; 00             .    
0057: ld b,(hl)                      ; 46             f    
0058: ld l,c                         ; 69             i    
0059: ld l,h                         ; 6c             l    
005a: ld h,l                         ; 65             e    
005b: jr nz,.+116                    ; 20 74          .t   
005d: ld l,a                         ; 6f             o    
005e: ld l,a                         ; 6f             o    
005f: jr nz,.+108                    ; 20 6c          .l   
0061: ld h,c                         ; 61             a    
0062: ld (hl),d                      ; 72             r    
0063: ld h,a                         ; 67             g    
0064: ld h,l                         ; 65             e    
0065: nop                            ; 00             .    
0066: ld d,h                         ; 54             t    
0067: ld h,l                         ; 65             e    
0068: ld a,b                         ; 78             x    
0069: ld (hl),h                      ; 74             t    
006a: jr nz,.+102                    ; 20 66          .f   
006c: ld l,c                         ; 69             i    
006d: ld l,h                         ; 6c             l    
006e: ld h,l                         ; 65             e    
006f: jr nz,.+98                     ; 20 62          .b   
0071: ld (hl),l                      ; 75             u    
0072: ld (hl),e                      ; 73             s    
0073: ld a,c                         ; 79             y    
0074: nop                            ; 00             .    
0075: ld c,(hl)                      ; 4e             n    
0076: ld l,a                         ; 6f             o    
0077: ld (hl),h                      ; 74             t    
0078: jr nz,.+97                     ; 20 61          .a   
007a: jr nz,.+116                    ; 20 74          .t   
007c: ld a,c                         ; 79             y    
007d: ld (hl),b                      ; 70             p    
007e: ld h,l                         ; 65             e    
007f: ld (hl),a                      ; 77             w    
0080: ld (hl),d                      ; 72             r    
0081: ld l,c                         ; 69             i    
0082: ld (hl),h                      ; 74             t    
0083: ld h,l                         ; 65             e    
0084: ld (hl),d                      ; 72             r    
0085: nop                            ; 00             .    
0086: ld d,h                         ; 54             t    
0087: ld l,a                         ; 6f             o    
0088: ld l,a                         ; 6f             o    
0089: jr nz,.+109                    ; 20 6d          .m   
008b: ld h,c                         ; 61             a    
008c: ld l,(hl)                      ; 6e             n    
008d: ld a,c                         ; 79             y    
008e: jr nz,.+111                    ; 20 6f          .o   
0090: ld (hl),b                      ; 70             p    
0091: ld h,l                         ; 65             e    
0092: ld l,(hl)                      ; 6e             n    
0093: jr nz,.+102                    ; 20 66          .f   
0095: ld l,c                         ; 69             i    
0096: ld l,h                         ; 6c             l    
0097: ld h,l                         ; 65             e    
0098: ld (hl),e                      ; 73             s    
0099: nop                            ; 00             .    
009a: ld b,(hl)                      ; 46             f    
009b: ld l,c                         ; 69             i    
009c: ld l,h                         ; 6c             l    
009d: ld h,l                         ; 65             e    
009e: jr nz,.+116                    ; 20 74          .t   
00a0: ld h,c                         ; 61             a    
00a1: ld h,d                         ; 62             b    
00a2: ld l,h                         ; 6c             l    
00a3: ld h,l                         ; 65             e    
00a4: jr nz,.+111                    ; 20 6f          .o   
00a6: halt                           ; 76             v    
00a7: ld h,l                         ; 65             e    
00a8: ld (hl),d                      ; 72             r    
00a9: ld h,(hl)                      ; 66             f    
00aa: ld l,h                         ; 6c             l    
00ab: ld l,a                         ; 6f             o    
00ac: ld (hl),a                      ; 77             w    
00ad: nop                            ; 00             .    
00ae: ld c,c                         ; 49             i    
00af: ld l,(hl)                      ; 6e             n    
00b0: halt                           ; 76             v    
00b1: ld h,c                         ; 61             a    
00b2: ld l,h                         ; 6c             l    
00b3: ld l,c                         ; 69             i    
00b4: ld h,h                         ; 64             d    
00b5: jr nz,.+97                     ; 20 61          .a   
00b7: ld (hl),d                      ; 72             r    
00b8: ld h,a                         ; 67             g    
00b9: ld (hl),l                      ; 75             u    
00ba: ld l,l                         ; 6d             m    
00bb: ld h,l                         ; 65             e    
00bc: ld l,(hl)                      ; 6e             n    
00bd: ld (hl),h                      ; 74             t    
00be: nop                            ; 00             .    
00bf: ld c,c                         ; 49             i    
00c0: ld (hl),e                      ; 73             s    
00c1: jr nz,.+97                     ; 20 61          .a   
00c3: jr nz,.+100                    ; 20 64          .d   
00c5: ld l,c                         ; 69             i    
00c6: ld (hl),d                      ; 72             r    
00c7: ld h,l                         ; 65             e    
00c8: ld h,e                         ; 63             c    
00c9: ld (hl),h                      ; 74             t    
00ca: ld l,a                         ; 6f             o    
00cb: ld (hl),d                      ; 72             r    
00cc: ld a,c                         ; 79             y    
00cd: nop                            ; 00             .    
00ce: ld c,(hl)                      ; 4e             n    
00cf: ld l,a                         ; 6f             o    
00d0: ld (hl),h                      ; 74             t    
00d1: jr nz,.+97                     ; 20 61          .a   
00d3: jr nz,.+100                    ; 20 64          .d   
00d5: ld l,c                         ; 69             i    
00d6: ld (hl),d                      ; 72             r    
00d7: ld h,l                         ; 65             e    
00d8: ld h,e                         ; 63             c    
00d9: ld (hl),h                      ; 74             t    
00da: ld l,a                         ; 6f             o    
00db: ld (hl),d                      ; 72             r    
00dc: ld a,c                         ; 79             y    
00dd: nop                            ; 00             .    
00de: ld c,(hl)                      ; 4e             n    
00df: ld l,a                         ; 6f             o    
00e0: jr nz,.+115                    ; 20 73          .s   
00e2: ld (hl),l                      ; 75             u    
00e3: ld h,e                         ; 63             c    
00e4: ld l,b                         ; 68             h    
00e5: jr nz,.+100                    ; 20 64          .d   
00e7: ld h,l                         ; 65             e    
00e8: halt                           ; 76             v    
00e9: ld l,c                         ; 69             i    
00ea: ld h,e                         ; 63             c    
00eb: ld h,l                         ; 65             e    
00ec: nop                            ; 00             .    
00ed: ld b,e                         ; 43             c    
00ee: ld (hl),d                      ; 72             r    
00ef: ld l,a                         ; 6f             o    
00f0: ld (hl),e                      ; 73             s    
00f1: ld (hl),e                      ; 73             s    
00f2: dec l                          ; 2d             -    
00f3: ld h,h                         ; 64             d    
00f4: ld h,l                         ; 65             e    
00f5: halt                           ; 76             v    
00f6: ld l,c                         ; 69             i    
00f7: ld h,e                         ; 63             c    
00f8: ld h,l                         ; 65             e    
00f9: jr nz,.+108                    ; 20 6c          .l   
00fb: ld l,c                         ; 69             i    
00fc: ld l,(hl)                      ; 6e             n    
00fd: ld l,e                         ; 6b             k    
00fe: nop                            ; 00             .    
00ff: ld b,(hl)                      ; 46             f    
0100: ld l,c                         ; 69             i    
0101: ld l,h                         ; 6c             l    
0102: ld h,l                         ; 65             e    
0103: jr nz,.+101                    ; 20 65          .e   
0105: ld a,b                         ; 78             x    
0106: ld l,c                         ; 69             i    
0107: ld (hl),e                      ; 73             s    
0108: ld (hl),h                      ; 74             t    
0109: ld (hl),e                      ; 73             s    
010a: nop                            ; 00             .    
010b: ld b,(hl)                      ; 46             f    
010c: ld l,c                         ; 69             i    
010d: ld l,h                         ; 6c             l    
010e: ld h,l                         ; 65             e    
010f: jr nz,.+111                    ; 20 6f          .o   
0111: ld (hl),d                      ; 72             r    
0112: jr nz,.+100                    ; 20 64          .d   
0114: ld h,l                         ; 65             e    
0115: halt                           ; 76             v    
0116: ld l,c                         ; 69             i    
0117: ld h,e                         ; 63             c    
0118: ld h,l                         ; 65             e    
0119: jr nz,.+98                     ; 20 62          .b   
011b: ld (hl),l                      ; 75             u    
011c: ld (hl),e                      ; 73             s    
011d: ld a,c                         ; 79             y    
011e: nop                            ; 00             .    
011f: ld b,d                         ; 42             b    
0120: ld l,h                         ; 6c             l    
0121: ld l,a                         ; 6f             o    
0122: ld h,e                         ; 63             c    
0123: ld l,e                         ; 6b             k    
0124: jr nz,.+100                    ; 20 64          .d   
0126: ld h,l                         ; 65             e    
0127: halt                           ; 76             v    
0128: ld l,c                         ; 69             i    
0129: ld h,e                         ; 63             c    
012a: ld h,l                         ; 65             e    
012b: jr nz,.+114                    ; 20 72          .r   
012d: ld h,l                         ; 65             e    
012e: ld (hl),c                      ; 71             q    
012f: ld (hl),l                      ; 75             u    
0130: ld l,c                         ; 69             i    
0131: ld (hl),d                      ; 72             r    
0132: ld h,l                         ; 65             e    
0133: ld h,h                         ; 64             d    
0134: nop                            ; 00             .    
0135: nop                            ; 00             .    
0136: ld d,b                         ; 50             p    
0137: ld h,l                         ; 65             e    
0138: ld (hl),d                      ; 72             r    
0139: ld l,l                         ; 6d             m    
013a: ld l,c                         ; 69             i    
013b: ld (hl),e                      ; 73             s    
013c: ld (hl),e                      ; 73             s    
013d: ld l,c                         ; 69             i    
013e: ld l,a                         ; 6f             o    
013f: ld l,(hl)                      ; 6e             n    
0140: jr nz,.+100                    ; 20 64          .d   
0142: ld h,l                         ; 65             e    
0143: ld l,(hl)                      ; 6e             n    
0144: ld l,c                         ; 69             i    
0145: ld h,l                         ; 65             e    
0146: ld h,h                         ; 64             d    
0147: nop                            ; 00             .    
0148: ld c,(hl)                      ; 4e             n    
0149: ld l,a                         ; 6f             o    
014a: ld (hl),h                      ; 74             t    
014b: jr nz,.+101                    ; 20 65          .e   
014d: ld l,(hl)                      ; 6e             n    
014e: ld l,a                         ; 6f             o    
014f: ld (hl),l                      ; 75             u    
0150: ld h,a                         ; 67             g    
0151: ld l,b                         ; 68             h    
0152: jr nz,.+109                    ; 20 6d          .m   
0154: ld h,l                         ; 65             e    
0155: ld l,l                         ; 6d             m    
0156: ld l,a                         ; 6f             o    
0157: ld (hl),d                      ; 72             r    
0158: ld a,c                         ; 79             y    
0159: nop                            ; 00             .    
015a: ld c,(hl)                      ; 4e             n    
015b: ld l,a                         ; 6f             o    
015c: jr nz,.+109                    ; 20 6d          .m   
015e: ld l,a                         ; 6f             o    
015f: ld (hl),d                      ; 72             r    
0160: ld h,l                         ; 65             e    
0161: jr nz,.+112                    ; 20 70          .p   
0163: ld (hl),d                      ; 72             r    
0164: ld l,a                         ; 6f             o    
0165: ld h,e                         ; 63             c    
0166: ld h,l                         ; 65             e    
0167: ld (hl),e                      ; 73             s    
0168: ld (hl),e                      ; 73             s    
0169: ld h,l                         ; 65             e    
016a: ld (hl),e                      ; 73             s    
016b: nop                            ; 00             .    
016c: ld c,(hl)                      ; 4e             n    
016d: ld l,a                         ; 6f             o    
016e: jr nz,.+99                     ; 20 63          .c   
0170: ld l,b                         ; 68             h    
0171: ld l,c                         ; 69             i    
0172: ld l,h                         ; 6c             l    
0173: ld h,h                         ; 64             d    
0174: ld (hl),d                      ; 72             r    
0175: ld h,l                         ; 65             e    
0176: ld l,(hl)                      ; 6e             n    
0177: nop                            ; 00             .    
0178: ld b,d                         ; 42             b    
0179: ld h,c                         ; 61             a    
017a: ld h,h                         ; 64             d    
017b: jr nz,.+102                    ; 20 66          .f   
017d: ld l,c                         ; 69             i    
017e: ld l,h                         ; 6c             l    
017f: ld h,l                         ; 65             e    
0180: jr nz,.+110                    ; 20 6e          .n   
0182: ld (hl),l                      ; 75             u    
0183: ld l,l                         ; 6d             m    
0184: ld h,d                         ; 62             b    
0185: ld h,l                         ; 65             e    
0186: ld (hl),d                      ; 72             r    
0187: nop                            ; 00             .    
0188: ld b,l                         ; 45             e    
0189: ld a,b                         ; 78             x    
018a: ld h,l                         ; 65             e    
018b: ld h,e                         ; 63             c    
018c: jr nz,.+102                    ; 20 66          .f   
018e: ld l,a                         ; 6f             o    
018f: ld (hl),d                      ; 72             r    
0190: ld l,l                         ; 6d             m    
0191: ld h,c                         ; 61             a    
0192: ld (hl),h                      ; 74             t    
0193: jr nz,.+101                    ; 20 65          .e   
0195: ld (hl),d                      ; 72             r    
0196: ld (hl),d                      ; 72             r    
0197: ld l,a                         ; 6f             o    
0198: ld (hl),d                      ; 72             r    
0199: nop                            ; 00             .    
019a: ld b,c                         ; 41             a    
019b: ld (hl),d                      ; 72             r    
019c: ld h,a                         ; 67             g    
019d: jr nz,.+108                    ; 20 6c          .l   
019f: ld l,c                         ; 69             i    
01a0: ld (hl),e                      ; 73             s    
01a1: ld (hl),h                      ; 74             t    
01a2: jr nz,.+116                    ; 20 74          .t   
01a4: ld l,a                         ; 6f             o    
01a5: ld l,a                         ; 6f             o    
01a6: jr nz,.+108                    ; 20 6c          .l   
01a8: ld l,a                         ; 6f             o    
01a9: ld l,(hl)                      ; 6e             n    
01aa: ld h,a                         ; 67             g    
01ab: nop                            ; 00             .    
01ac: ld c,(hl)                      ; 4e             n    
01ad: ld l,a                         ; 6f             o    
01ae: jr nz,.+115                    ; 20 73          .s   
01b0: ld (hl),l                      ; 75             u    
01b1: ld h,e                         ; 63             c    
01b2: ld l,b                         ; 68             h    
01b3: jr nz,.+100                    ; 20 64          .d   
01b5: ld h,l                         ; 65             e    
01b6: halt                           ; 76             v    
01b7: ld l,c                         ; 69             i    
01b8: ld h,e                         ; 63             c    
01b9: ld h,l                         ; 65             e    
01ba: jr nz,.+111                    ; 20 6f          .o   
01bc: ld (hl),d                      ; 72             r    
01bd: jr nz,.+97                     ; 20 61          .a   
01bf: ld h,h                         ; 64             d    
01c0: ld h,h                         ; 64             d    
01c1: ld (hl),d                      ; 72             r    
01c2: ld h,l                         ; 65             e    
01c3: ld (hl),e                      ; 73             s    
01c4: ld (hl),e                      ; 73             s    
01c5: nop                            ; 00             .    
01c6: ld c,c                         ; 49             i    
01c7: cpl                            ; 2f             /    
01c8: ld c,a                         ; 4f             o    
01c9: jr nz,.+101                    ; 20 65          .e   
01cb: ld (hl),d                      ; 72             r    
01cc: ld (hl),d                      ; 72             r    
01cd: ld l,a                         ; 6f             o    
01ce: ld (hl),d                      ; 72             r    
01cf: nop                            ; 00             .    
01d0: ld c,c                         ; 49             i    
01d1: ld l,(hl)                      ; 6e             n    
01d2: ld (hl),h                      ; 74             t    
01d3: ld h,l                         ; 65             e    
01d4: ld (hl),d                      ; 72             r    
01d5: ld (hl),d                      ; 72             r    
01d6: ld (hl),l                      ; 75             u    
01d7: ld (hl),b                      ; 70             p    
01d8: ld (hl),h                      ; 74             t    
01d9: ld h,l                         ; 65             e    
01da: ld h,h                         ; 64             d    
01db: jr nz,.+115                    ; 20 73          .s   
01dd: ld a,c                         ; 79             y    
01de: ld (hl),e                      ; 73             s    
01df: ld (hl),h                      ; 74             t    
01e0: ld h,l                         ; 65             e    
01e1: ld l,l                         ; 6d             m    
01e2: jr nz,.+99                     ; 20 63          .c   
01e4: ld h,c                         ; 61             a    
01e5: ld l,h                         ; 6c             l    
01e6: ld l,h                         ; 6c             l    
01e7: nop                            ; 00             .    
01e8: ld c,(hl)                      ; 4e             n    
01e9: ld l,a                         ; 6f             o    
01ea: jr nz,.+115                    ; 20 73          .s   
01ec: ld (hl),l                      ; 75             u    
01ed: ld h,e                         ; 63             c    
01ee: ld l,b                         ; 68             h    
01ef: jr nz,.+112                    ; 20 70          .p   
01f1: ld (hl),d                      ; 72             r    
01f2: ld l,a                         ; 6f             o    
01f3: ld h,e                         ; 63             c    
01f4: ld h,l                         ; 65             e    
01f5: ld (hl),e                      ; 73             s    
01f6: ld (hl),e                      ; 73             s    
01f7: nop                            ; 00             .    
01f8: ld c,(hl)                      ; 4e             n    
01f9: ld l,a                         ; 6f             o    
01fa: jr nz,.+115                    ; 20 73          .s   
01fc: ld (hl),l                      ; 75             u    
01fd: ld h,e                         ; 63             c    
01fe: ld l,b                         ; 68             h    
01ff: jr nz,.+102                    ; 20 66          .f   
0201: ld l,c                         ; 69             i    
0202: ld l,h                         ; 6c             l    
0203: ld h,l                         ; 65             e    
0204: jr nz,.+111                    ; 20 6f          .o   
0206: ld (hl),d                      ; 72             r    
0207: jr nz,.+100                    ; 20 64          .d   
0209: ld l,c                         ; 69             i    
020a: ld (hl),d                      ; 72             r    
020b: ld h,l                         ; 65             e    
020c: ld h,e                         ; 63             c    
020d: ld (hl),h                      ; 74             t    
020e: ld l,a                         ; 6f             o    
020f: ld (hl),d                      ; 72             r    
0210: ld a,c                         ; 79             y    
0211: nop                            ; 00             .    
0212: ld c,(hl)                      ; 4e             n    
0213: ld l,a                         ; 6f             o    
0214: ld (hl),h                      ; 74             t    
0215: jr nz,.+115                    ; 20 73          .s   
0217: ld (hl),l                      ; 75             u    
0218: ld (hl),b                      ; 70             p    
0219: ld h,l                         ; 65             e    
021a: ld (hl),d                      ; 72             r    
021b: dec l                          ; 2d             -    
021c: ld (hl),l                      ; 75             u    
021d: ld (hl),e                      ; 73             s    
021e: ld h,l                         ; 65             e    
021f: ld (hl),d                      ; 72             r    
0220: nop                            ; 00             .    
0221: ld d,l                         ; 55             u    
0222: ld l,(hl)                      ; 6e             n    
0223: ld l,e                         ; 6b             k    
0224: ld l,(hl)                      ; 6e             n    
0225: ld l,a                         ; 6f             o    
0226: ld (hl),a                      ; 77             w    
0227: ld l,(hl)                      ; 6e             n    
0228: jr nz,.+101                    ; 20 65          .e   
022a: ld (hl),d                      ; 72             r    
022b: ld (hl),d                      ; 72             r    
022c: ld l,a                         ; 6f             o    
022d: ld (hl),d                      ; 72             r    
022e: nop                            ; 00             .    
022f: ld a,(bc)                      ; 0a             .    
0230: nop                            ; 00             .    
0231: ld a,(0x20)                    ; 3a 20 00       :..  
0234: call 0x0                       ; cd 00 00       ...  
0237: push af                        ; f5             .    
0238: push af                        ; f5             .    
0239: push af                        ; f5             .    
023a: push af                        ; f5             .    
023b: ld hl,0x4                      ; 21 04 00       !..  
023e: add hl,de                      ; 19             .    
023f: ld a,(hl)                      ; 7e             ~    
0240: inc hl                         ; 23             #    
0241: or a,(hl)                      ; b6             .    
0242: jp z,0x283                     ; ca 83 02       ...  
0245: ld hl,0x4                      ; 21 04 00       !..  
0248: add hl,de                      ; 19             .    
0249: ld a,(hl)                      ; 7e             ~    
024a: inc hl                         ; 23             #    
024b: ld h,(hl)                      ; 66             f    
024c: ld l,a                         ; 6f             o    
024d: ld a,(hl)                      ; 7e             ~    
024e: or a,a                         ; b7             .    
024f: jp z,0x283                     ; ca 83 02       ...  
0252: ld hl,0x4                      ; 21 04 00       !..  
0255: add hl,de                      ; 19             .    
0256: ld c,(hl)                      ; 4e             n    
0257: inc hl                         ; 23             #    
0258: ld b,(hl)                      ; 46             f    
0259: push bc                        ; c5             .    
025a: call 0x2fb                     ; cd fb 02       ...  
025d: pop af                         ; f1             .    
025e: push bc                        ; c5             .    
025f: ld hl,0x4                      ; 21 04 00       !..  
0262: add hl,de                      ; 19             .    
0263: ld c,(hl)                      ; 4e             n    
0264: inc hl                         ; 23             #    
0265: ld b,(hl)                      ; 46             f    
0266: push bc                        ; c5             .    
0267: ld hl,0x2                      ; 21 02 00       !..  
026a: push hl                        ; e5             .    
026b: call 0x0                       ; cd 00 00       ...  
026e: pop af                         ; f1             .    
026f: pop af                         ; f1             .    
0270: pop af                         ; f1             .    
0271: ld hl,0x2                      ; 21 02 00       !..  
0274: push hl                        ; e5             .    
0275: ld hl,0x231                    ; 21 31 02       !1.  
0278: push hl                        ; e5             .    
0279: ld hl,0x2                      ; 21 02 00       !..  
027c: push hl                        ; e5             .    
027d: call 0x0                       ; cd 00 00       ...  
0280: pop af                         ; f1             .    
0281: pop af                         ; f1             .    
0282: pop af                         ; f1             .    
0283: ld a,(0x0)                     ; 3a 00 00       :..  
0286: sub a,0x1                      ; d6 01          ..   
0288: ld a,(0x1)                     ; 3a 01 00       :..  
028b: sbc a,0x0                      ; de 00          ..   
028d: jp m,0x2ad                     ; fa ad 02       ...  
0290: ld hl,0x0                      ; 21 00 00       !..  
0293: ld a,0x20                      ; 3e 20          >.   
0295: sub a,(hl)                     ; 96             .    
0296: ld a,0x0                       ; 3e 00          >.   
0298: inc hl                         ; 23             #    
0299: sbc a,(hl)                     ; 9e             .    
029a: jp m,0x2ad                     ; fa ad 02       ...  
029d: ld hl,0xfff8                   ; 21 f8 ff       !..  
02a0: add hl,de                      ; 19             .    
02a1: ld a,(0x0)                     ; 3a 00 00       :..  
02a4: ld (hl),a                      ; 77             w    
02a5: ld a,(0x1)                     ; 3a 01 00       :..  
02a8: inc hl                         ; 23             #    
02a9: ld (hl),a                      ; 77             w    
02aa: jp 0x2b5                       ; c3 b5 02       ...  
02ad: ld hl,0xfff8                   ; 21 f8 ff       !..  
02b0: add hl,de                      ; 19             .    
02b1: sub a,a                        ; 97             .    
02b2: ld (hl),a                      ; 77             w    
02b3: inc hl                         ; 23             #    
02b4: ld (hl),a                      ; 77             w    
02b5: ld hl,0xfff8                   ; 21 f8 ff       !..  
02b8: add hl,de                      ; 19             .    
02b9: ld a,(hl)                      ; 7e             ~    
02ba: inc hl                         ; 23             #    
02bb: ld h,(hl)                      ; 66             f    
02bc: ld l,a                         ; 6f             o    
02bd: add hl,hl                      ; 29             )    
02be: ld bc,0x333                    ; 01 33 03       .3.  
02c1: add hl,bc                      ; 09             .    
02c2: ld c,(hl)                      ; 4e             n    
02c3: inc hl                         ; 23             #    
02c4: ld b,(hl)                      ; 46             f    
02c5: push bc                        ; c5             .    
02c6: call 0x2fb                     ; cd fb 02       ...  
02c9: pop af                         ; f1             .    
02ca: push bc                        ; c5             .    
02cb: ld hl,0xfff8                   ; 21 f8 ff       !..  
02ce: add hl,de                      ; 19             .    
02cf: ld a,(hl)                      ; 7e             ~    
02d0: inc hl                         ; 23             #    
02d1: ld h,(hl)                      ; 66             f    
02d2: ld l,a                         ; 6f             o    
02d3: add hl,hl                      ; 29             )    
02d4: ld bc,0x333                    ; 01 33 03       .3.  
02d7: add hl,bc                      ; 09             .    
02d8: ld c,(hl)                      ; 4e             n    
02d9: inc hl                         ; 23             #    
02da: ld b,(hl)                      ; 46             f    
02db: push bc                        ; c5             .    
02dc: ld hl,0x2                      ; 21 02 00       !..  
02df: push hl                        ; e5             .    
02e0: call 0x0                       ; cd 00 00       ...  
02e3: pop af                         ; f1             .    
02e4: pop af                         ; f1             .    
02e5: pop af                         ; f1             .    
02e6: ld hl,0x1                      ; 21 01 00       !..  
02e9: push hl                        ; e5             .    
02ea: ld hl,0x22f                    ; 21 2f 02       !/.  
02ed: push hl                        ; e5             .    
02ee: ld hl,0x2                      ; 21 02 00       !..  
02f1: push hl                        ; e5             .    
02f2: call 0x0                       ; cd 00 00       ...  
02f5: pop af                         ; f1             .    
02f6: pop af                         ; f1             .    
02f7: pop af                         ; f1             .    
02f8: jp 0x0                         ; c3 00 00       ...  
02fb: call 0x0                       ; cd 00 00       ...  
02fe: ld hl,0x4                      ; 21 04 00       !..  
0301: add hl,de                      ; 19             .    
0302: ld a,(hl)                      ; 7e             ~    
0303: inc hl                         ; 23             #    
0304: ld h,(hl)                      ; 66             f    
0305: ld l,a                         ; 6f             o    
0306: ld (0x0),hl                    ; 22 00 00       "..  
0309: ld hl,(0x0)                    ; 2a 00 00       *..  
030c: ld (0x377),hl                  ; 22 77 03       "w.  
030f: ld hl,(0x377)                  ; 2a 77 03       *w.  
0312: ld a,(hl)                      ; 7e             ~    
0313: or a,a                         ; b7             .    
0314: jp z,0x321                     ; ca 21 03       .!.  
0317: ld hl,(0x377)                  ; 2a 77 03       *w.  
031a: inc hl                         ; 23             #    
031b: ld (0x377),hl                  ; 22 77 03       "w.  
031e: jp 0x30f                       ; c3 0f 03       ...  
0321: ld hl,(0x377)                  ; 2a 77 03       *w.  
0324: push hl                        ; e5             .    
0325: ld hl,0x0                      ; 21 00 00       !..  
0328: pop bc                         ; c1             .    
0329: ld a,c                         ; 79             y    
032a: sub a,(hl)                     ; 96             .    
032b: ld c,a                         ; 4f             o    
032c: ld a,b                         ; 78             x    
032d: inc hl                         ; 23             #    
032e: sbc a,(hl)                     ; 9e             .    
032f: ld b,a                         ; 47             g    
0330: jp 0x0                         ; c3 00 00       ...  
data:
0333: 21 02 12 02  f8 01 e8 01  d0 01 c6 01  ac 01 9a 01  !... .... .... .... 
0343: 88 01 78 01  6c 01 5a 01  48 01 36 01  35 01 1f 01  ..x. l.z. h.6. 5... 
0353: 0b 01 ff 00  ed 00 de 00  ce 00 bf 00  ae 00 9a 00  .... .... .... .... 
0363: 86 00 75 00  66 00 57 00  3f 00 32 00  1c 00 0d 00  ..u. f.w. ?.2. .... 
0373: 01 00 00 00  00 00                               .... ..
