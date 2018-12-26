../filesystem/lib/libu.a perror.o:
    0      c.r4: 0000 global 
    1     c.ret: 0000 global 
    2     c.ent: 0000 global 
    3    c.rets: 0000 global 
    4    c.ents: 0000 global 
    5    _write: 0000 global 
    6    _errno: 0000 global 
    7   _perror: 0234 global defined code 
DATA:0042 text relative
DATA:0040 text relative
DATA:003e text relative
DATA:003c text relative
DATA:003a text relative
DATA:0038 text relative
DATA:0036 text relative
DATA:0034 text relative
DATA:0032 text relative
DATA:0030 text relative
DATA:002e text relative
DATA:002c text relative
DATA:002a text relative
DATA:0028 text relative
DATA:0026 text relative
DATA:0024 text relative
DATA:0022 text relative
DATA:0020 text relative
DATA:001e text relative
DATA:001c text relative
DATA:001a text relative
DATA:0018 text relative
DATA:0016 text relative
DATA:0014 text relative
DATA:0012 text relative
DATA:0010 text relative
DATA:000e text relative
DATA:000c text relative
DATA:000a text relative
DATA:0008 text relative
DATA:0006 text relative
DATA:0004 text relative
DATA:0002 text relative
DATA:0000 text relative
TEXT:0331 symbol reference c.rets
TEXT:0326 symbol reference c.r4
TEXT:0322 data relative
TEXT:031f text relative
TEXT:031c data relative
TEXT:0318 data relative
TEXT:0315 text relative
TEXT:0310 data relative
TEXT:030d data relative
TEXT:030a symbol reference c.r4
TEXT:0307 symbol reference c.r4
TEXT:02fc symbol reference c.ents
TEXT:02f9 symbol reference c.ret
TEXT:02f3 symbol reference _write
TEXT:02eb text relative
TEXT:02e1 symbol reference _write
TEXT:02d5 data relative
TEXT:02c7 text relative
TEXT:02bf data relative
TEXT:02ab text relative
TEXT:02a6 symbol reference _errno
TEXT:02a2 symbol reference _errno
TEXT:029b text relative
TEXT:0291 symbol reference _errno
TEXT:028e text relative
TEXT:0289 symbol reference _errno
TEXT:0284 symbol reference _errno
TEXT:027e symbol reference _write
TEXT:0276 text relative
TEXT:026c symbol reference _write
TEXT:025b text relative
TEXT:0250 text relative
TEXT:0243 text relative
TEXT:0235 symbol reference c.ent
0000: NOP                            ; 00             .    
0001: LD B,D                         ; 42             B    
0002: LD (HL),D                      ; 72             r    
0003: LD L,A                         ; 6f             o    
0004: LD L,E                         ; 6b             k    
0005: LD H,L                         ; 65             e    
0006: LD L,(HL)                      ; 6e             n    
0007: JR NZ,.+112                    ; 20 70          .p   
0009: LD L,C                         ; 69             i    
000a: LD (HL),B                      ; 70             p    
000b: LD H,L                         ; 65             e    
000c: NOP                            ; 00             .    
000d: LD D,H                         ; 54             T    
000e: LD L,A                         ; 6f             o    
000f: LD L,A                         ; 6f             o    
0010: JR NZ,.+109                    ; 20 6d          .m   
0012: LD H,C                         ; 61             a    
0013: LD L,(HL)                      ; 6e             n    
0014: LD A,C                         ; 79             y    
0015: JR NZ,.+108                    ; 20 6c          .l   
0017: LD L,C                         ; 69             i    
0018: LD L,(HL)                      ; 6e             n    
0019: LD L,E                         ; 6b             k    
001a: LD (HL),E                      ; 73             s    
001b: NOP                            ; 00             .    
001c: LD D,D                         ; 52             R    
001d: LD H,L                         ; 65             e    
001e: LD H,C                         ; 61             a    
001f: LD H,H                         ; 64             d    
0020: DEC L                          ; 2d             -    
0021: LD L,A                         ; 6f             o    
0022: LD L,(HL)                      ; 6e             n    
0023: LD L,H                         ; 6c             l    
0024: LD A,C                         ; 79             y    
0025: JR NZ,.+102                    ; 20 66          .f   
0027: LD L,C                         ; 69             i    
0028: LD L,H                         ; 6c             l    
0029: LD H,L                         ; 65             e    
002a: JR NZ,.+115                    ; 20 73          .s   
002c: LD A,C                         ; 79             y    
002d: LD (HL),E                      ; 73             s    
002e: LD (HL),H                      ; 74             t    
002f: LD H,L                         ; 65             e    
0030: LD L,L                         ; 6d             m    
0031: NOP                            ; 00             .    
0032: LD C,C                         ; 49             I    
0033: LD L,H                         ; 6c             l    
0034: LD L,H                         ; 6c             l    
0035: LD H,L                         ; 65             e    
0036: LD H,A                         ; 67             g    
0037: LD H,C                         ; 61             a    
0038: LD L,H                         ; 6c             l    
0039: JR NZ,.+115                    ; 20 73          .s   
003b: LD H,L                         ; 65             e    
003c: LD H,L                         ; 65             e    
003d: LD L,E                         ; 6b             k    
003e: NOP                            ; 00             .    
003f: LD C,(HL)                      ; 4e             N    
0040: LD L,A                         ; 6f             o    
0041: JR NZ,.+115                    ; 20 73          .s   
0043: LD (HL),B                      ; 70             p    
0044: LD H,C                         ; 61             a    
0045: LD H,E                         ; 63             c    
0046: LD H,L                         ; 65             e    
0047: JR NZ,.+108                    ; 20 6c          .l   
0049: LD H,L                         ; 65             e    
004a: LD H,(HL)                      ; 66             f    
004b: LD (HL),H                      ; 74             t    
004c: JR NZ,.+111                    ; 20 6f          .o   
004e: LD L,(HL)                      ; 6e             n    
004f: JR NZ,.+100                    ; 20 64          .d   
0051: LD H,L                         ; 65             e    
0052: HALT                           ; 76             v    
0053: LD L,C                         ; 69             i    
0054: LD H,E                         ; 63             c    
0055: LD H,L                         ; 65             e    
0056: NOP                            ; 00             .    
0057: LD B,(HL)                      ; 46             F    
0058: LD L,C                         ; 69             i    
0059: LD L,H                         ; 6c             l    
005a: LD H,L                         ; 65             e    
005b: JR NZ,.+116                    ; 20 74          .t   
005d: LD L,A                         ; 6f             o    
005e: LD L,A                         ; 6f             o    
005f: JR NZ,.+108                    ; 20 6c          .l   
0061: LD H,C                         ; 61             a    
0062: LD (HL),D                      ; 72             r    
0063: LD H,A                         ; 67             g    
0064: LD H,L                         ; 65             e    
0065: NOP                            ; 00             .    
0066: LD D,H                         ; 54             T    
0067: LD H,L                         ; 65             e    
0068: LD A,B                         ; 78             x    
0069: LD (HL),H                      ; 74             t    
006a: JR NZ,.+102                    ; 20 66          .f   
006c: LD L,C                         ; 69             i    
006d: LD L,H                         ; 6c             l    
006e: LD H,L                         ; 65             e    
006f: JR NZ,.+98                     ; 20 62          .b   
0071: LD (HL),L                      ; 75             u    
0072: LD (HL),E                      ; 73             s    
0073: LD A,C                         ; 79             y    
0074: NOP                            ; 00             .    
0075: LD C,(HL)                      ; 4e             N    
0076: LD L,A                         ; 6f             o    
0077: LD (HL),H                      ; 74             t    
0078: JR NZ,.+97                     ; 20 61          .a   
007a: JR NZ,.+116                    ; 20 74          .t   
007c: LD A,C                         ; 79             y    
007d: LD (HL),B                      ; 70             p    
007e: LD H,L                         ; 65             e    
007f: LD (HL),A                      ; 77             w    
0080: LD (HL),D                      ; 72             r    
0081: LD L,C                         ; 69             i    
0082: LD (HL),H                      ; 74             t    
0083: LD H,L                         ; 65             e    
0084: LD (HL),D                      ; 72             r    
0085: NOP                            ; 00             .    
0086: LD D,H                         ; 54             T    
0087: LD L,A                         ; 6f             o    
0088: LD L,A                         ; 6f             o    
0089: JR NZ,.+109                    ; 20 6d          .m   
008b: LD H,C                         ; 61             a    
008c: LD L,(HL)                      ; 6e             n    
008d: LD A,C                         ; 79             y    
008e: JR NZ,.+111                    ; 20 6f          .o   
0090: LD (HL),B                      ; 70             p    
0091: LD H,L                         ; 65             e    
0092: LD L,(HL)                      ; 6e             n    
0093: JR NZ,.+102                    ; 20 66          .f   
0095: LD L,C                         ; 69             i    
0096: LD L,H                         ; 6c             l    
0097: LD H,L                         ; 65             e    
0098: LD (HL),E                      ; 73             s    
0099: NOP                            ; 00             .    
009a: LD B,(HL)                      ; 46             F    
009b: LD L,C                         ; 69             i    
009c: LD L,H                         ; 6c             l    
009d: LD H,L                         ; 65             e    
009e: JR NZ,.+116                    ; 20 74          .t   
00a0: LD H,C                         ; 61             a    
00a1: LD H,D                         ; 62             b    
00a2: LD L,H                         ; 6c             l    
00a3: LD H,L                         ; 65             e    
00a4: JR NZ,.+111                    ; 20 6f          .o   
00a6: HALT                           ; 76             v    
00a7: LD H,L                         ; 65             e    
00a8: LD (HL),D                      ; 72             r    
00a9: LD H,(HL)                      ; 66             f    
00aa: LD L,H                         ; 6c             l    
00ab: LD L,A                         ; 6f             o    
00ac: LD (HL),A                      ; 77             w    
00ad: NOP                            ; 00             .    
00ae: LD C,C                         ; 49             I    
00af: LD L,(HL)                      ; 6e             n    
00b0: HALT                           ; 76             v    
00b1: LD H,C                         ; 61             a    
00b2: LD L,H                         ; 6c             l    
00b3: LD L,C                         ; 69             i    
00b4: LD H,H                         ; 64             d    
00b5: JR NZ,.+97                     ; 20 61          .a   
00b7: LD (HL),D                      ; 72             r    
00b8: LD H,A                         ; 67             g    
00b9: LD (HL),L                      ; 75             u    
00ba: LD L,L                         ; 6d             m    
00bb: LD H,L                         ; 65             e    
00bc: LD L,(HL)                      ; 6e             n    
00bd: LD (HL),H                      ; 74             t    
00be: NOP                            ; 00             .    
00bf: LD C,C                         ; 49             I    
00c0: LD (HL),E                      ; 73             s    
00c1: JR NZ,.+97                     ; 20 61          .a   
00c3: JR NZ,.+100                    ; 20 64          .d   
00c5: LD L,C                         ; 69             i    
00c6: LD (HL),D                      ; 72             r    
00c7: LD H,L                         ; 65             e    
00c8: LD H,E                         ; 63             c    
00c9: LD (HL),H                      ; 74             t    
00ca: LD L,A                         ; 6f             o    
00cb: LD (HL),D                      ; 72             r    
00cc: LD A,C                         ; 79             y    
00cd: NOP                            ; 00             .    
00ce: LD C,(HL)                      ; 4e             N    
00cf: LD L,A                         ; 6f             o    
00d0: LD (HL),H                      ; 74             t    
00d1: JR NZ,.+97                     ; 20 61          .a   
00d3: JR NZ,.+100                    ; 20 64          .d   
00d5: LD L,C                         ; 69             i    
00d6: LD (HL),D                      ; 72             r    
00d7: LD H,L                         ; 65             e    
00d8: LD H,E                         ; 63             c    
00d9: LD (HL),H                      ; 74             t    
00da: LD L,A                         ; 6f             o    
00db: LD (HL),D                      ; 72             r    
00dc: LD A,C                         ; 79             y    
00dd: NOP                            ; 00             .    
00de: LD C,(HL)                      ; 4e             N    
00df: LD L,A                         ; 6f             o    
00e0: JR NZ,.+115                    ; 20 73          .s   
00e2: LD (HL),L                      ; 75             u    
00e3: LD H,E                         ; 63             c    
00e4: LD L,B                         ; 68             h    
00e5: JR NZ,.+100                    ; 20 64          .d   
00e7: LD H,L                         ; 65             e    
00e8: HALT                           ; 76             v    
00e9: LD L,C                         ; 69             i    
00ea: LD H,E                         ; 63             c    
00eb: LD H,L                         ; 65             e    
00ec: NOP                            ; 00             .    
00ed: LD B,E                         ; 43             C    
00ee: LD (HL),D                      ; 72             r    
00ef: LD L,A                         ; 6f             o    
00f0: LD (HL),E                      ; 73             s    
00f1: LD (HL),E                      ; 73             s    
00f2: DEC L                          ; 2d             -    
00f3: LD H,H                         ; 64             d    
00f4: LD H,L                         ; 65             e    
00f5: HALT                           ; 76             v    
00f6: LD L,C                         ; 69             i    
00f7: LD H,E                         ; 63             c    
00f8: LD H,L                         ; 65             e    
00f9: JR NZ,.+108                    ; 20 6c          .l   
00fb: LD L,C                         ; 69             i    
00fc: LD L,(HL)                      ; 6e             n    
00fd: LD L,E                         ; 6b             k    
00fe: NOP                            ; 00             .    
00ff: LD B,(HL)                      ; 46             F    
0100: LD L,C                         ; 69             i    
0101: LD L,H                         ; 6c             l    
0102: LD H,L                         ; 65             e    
0103: JR NZ,.+101                    ; 20 65          .e   
0105: LD A,B                         ; 78             x    
0106: LD L,C                         ; 69             i    
0107: LD (HL),E                      ; 73             s    
0108: LD (HL),H                      ; 74             t    
0109: LD (HL),E                      ; 73             s    
010a: NOP                            ; 00             .    
010b: LD B,(HL)                      ; 46             F    
010c: LD L,C                         ; 69             i    
010d: LD L,H                         ; 6c             l    
010e: LD H,L                         ; 65             e    
010f: JR NZ,.+111                    ; 20 6f          .o   
0111: LD (HL),D                      ; 72             r    
0112: JR NZ,.+100                    ; 20 64          .d   
0114: LD H,L                         ; 65             e    
0115: HALT                           ; 76             v    
0116: LD L,C                         ; 69             i    
0117: LD H,E                         ; 63             c    
0118: LD H,L                         ; 65             e    
0119: JR NZ,.+98                     ; 20 62          .b   
011b: LD (HL),L                      ; 75             u    
011c: LD (HL),E                      ; 73             s    
011d: LD A,C                         ; 79             y    
011e: NOP                            ; 00             .    
011f: LD B,D                         ; 42             B    
0120: LD L,H                         ; 6c             l    
0121: LD L,A                         ; 6f             o    
0122: LD H,E                         ; 63             c    
0123: LD L,E                         ; 6b             k    
0124: JR NZ,.+100                    ; 20 64          .d   
0126: LD H,L                         ; 65             e    
0127: HALT                           ; 76             v    
0128: LD L,C                         ; 69             i    
0129: LD H,E                         ; 63             c    
012a: LD H,L                         ; 65             e    
012b: JR NZ,.+114                    ; 20 72          .r   
012d: LD H,L                         ; 65             e    
012e: LD (HL),C                      ; 71             q    
012f: LD (HL),L                      ; 75             u    
0130: LD L,C                         ; 69             i    
0131: LD (HL),D                      ; 72             r    
0132: LD H,L                         ; 65             e    
0133: LD H,H                         ; 64             d    
0134: NOP                            ; 00             .    
0135: NOP                            ; 00             .    
0136: LD D,B                         ; 50             P    
0137: LD H,L                         ; 65             e    
0138: LD (HL),D                      ; 72             r    
0139: LD L,L                         ; 6d             m    
013a: LD L,C                         ; 69             i    
013b: LD (HL),E                      ; 73             s    
013c: LD (HL),E                      ; 73             s    
013d: LD L,C                         ; 69             i    
013e: LD L,A                         ; 6f             o    
013f: LD L,(HL)                      ; 6e             n    
0140: JR NZ,.+100                    ; 20 64          .d   
0142: LD H,L                         ; 65             e    
0143: LD L,(HL)                      ; 6e             n    
0144: LD L,C                         ; 69             i    
0145: LD H,L                         ; 65             e    
0146: LD H,H                         ; 64             d    
0147: NOP                            ; 00             .    
0148: LD C,(HL)                      ; 4e             N    
0149: LD L,A                         ; 6f             o    
014a: LD (HL),H                      ; 74             t    
014b: JR NZ,.+101                    ; 20 65          .e   
014d: LD L,(HL)                      ; 6e             n    
014e: LD L,A                         ; 6f             o    
014f: LD (HL),L                      ; 75             u    
0150: LD H,A                         ; 67             g    
0151: LD L,B                         ; 68             h    
0152: JR NZ,.+109                    ; 20 6d          .m   
0154: LD H,L                         ; 65             e    
0155: LD L,L                         ; 6d             m    
0156: LD L,A                         ; 6f             o    
0157: LD (HL),D                      ; 72             r    
0158: LD A,C                         ; 79             y    
0159: NOP                            ; 00             .    
015a: LD C,(HL)                      ; 4e             N    
015b: LD L,A                         ; 6f             o    
015c: JR NZ,.+109                    ; 20 6d          .m   
015e: LD L,A                         ; 6f             o    
015f: LD (HL),D                      ; 72             r    
0160: LD H,L                         ; 65             e    
0161: JR NZ,.+112                    ; 20 70          .p   
0163: LD (HL),D                      ; 72             r    
0164: LD L,A                         ; 6f             o    
0165: LD H,E                         ; 63             c    
0166: LD H,L                         ; 65             e    
0167: LD (HL),E                      ; 73             s    
0168: LD (HL),E                      ; 73             s    
0169: LD H,L                         ; 65             e    
016a: LD (HL),E                      ; 73             s    
016b: NOP                            ; 00             .    
016c: LD C,(HL)                      ; 4e             N    
016d: LD L,A                         ; 6f             o    
016e: JR NZ,.+99                     ; 20 63          .c   
0170: LD L,B                         ; 68             h    
0171: LD L,C                         ; 69             i    
0172: LD L,H                         ; 6c             l    
0173: LD H,H                         ; 64             d    
0174: LD (HL),D                      ; 72             r    
0175: LD H,L                         ; 65             e    
0176: LD L,(HL)                      ; 6e             n    
0177: NOP                            ; 00             .    
0178: LD B,D                         ; 42             B    
0179: LD H,C                         ; 61             a    
017a: LD H,H                         ; 64             d    
017b: JR NZ,.+102                    ; 20 66          .f   
017d: LD L,C                         ; 69             i    
017e: LD L,H                         ; 6c             l    
017f: LD H,L                         ; 65             e    
0180: JR NZ,.+110                    ; 20 6e          .n   
0182: LD (HL),L                      ; 75             u    
0183: LD L,L                         ; 6d             m    
0184: LD H,D                         ; 62             b    
0185: LD H,L                         ; 65             e    
0186: LD (HL),D                      ; 72             r    
0187: NOP                            ; 00             .    
0188: LD B,L                         ; 45             E    
0189: LD A,B                         ; 78             x    
018a: LD H,L                         ; 65             e    
018b: LD H,E                         ; 63             c    
018c: JR NZ,.+102                    ; 20 66          .f   
018e: LD L,A                         ; 6f             o    
018f: LD (HL),D                      ; 72             r    
0190: LD L,L                         ; 6d             m    
0191: LD H,C                         ; 61             a    
0192: LD (HL),H                      ; 74             t    
0193: JR NZ,.+101                    ; 20 65          .e   
0195: LD (HL),D                      ; 72             r    
0196: LD (HL),D                      ; 72             r    
0197: LD L,A                         ; 6f             o    
0198: LD (HL),D                      ; 72             r    
0199: NOP                            ; 00             .    
019a: LD B,C                         ; 41             A    
019b: LD (HL),D                      ; 72             r    
019c: LD H,A                         ; 67             g    
019d: JR NZ,.+108                    ; 20 6c          .l   
019f: LD L,C                         ; 69             i    
01a0: LD (HL),E                      ; 73             s    
01a1: LD (HL),H                      ; 74             t    
01a2: JR NZ,.+116                    ; 20 74          .t   
01a4: LD L,A                         ; 6f             o    
01a5: LD L,A                         ; 6f             o    
01a6: JR NZ,.+108                    ; 20 6c          .l   
01a8: LD L,A                         ; 6f             o    
01a9: LD L,(HL)                      ; 6e             n    
01aa: LD H,A                         ; 67             g    
01ab: NOP                            ; 00             .    
01ac: LD C,(HL)                      ; 4e             N    
01ad: LD L,A                         ; 6f             o    
01ae: JR NZ,.+115                    ; 20 73          .s   
01b0: LD (HL),L                      ; 75             u    
01b1: LD H,E                         ; 63             c    
01b2: LD L,B                         ; 68             h    
01b3: JR NZ,.+100                    ; 20 64          .d   
01b5: LD H,L                         ; 65             e    
01b6: HALT                           ; 76             v    
01b7: LD L,C                         ; 69             i    
01b8: LD H,E                         ; 63             c    
01b9: LD H,L                         ; 65             e    
01ba: JR NZ,.+111                    ; 20 6f          .o   
01bc: LD (HL),D                      ; 72             r    
01bd: JR NZ,.+97                     ; 20 61          .a   
01bf: LD H,H                         ; 64             d    
01c0: LD H,H                         ; 64             d    
01c1: LD (HL),D                      ; 72             r    
01c2: LD H,L                         ; 65             e    
01c3: LD (HL),E                      ; 73             s    
01c4: LD (HL),E                      ; 73             s    
01c5: NOP                            ; 00             .    
01c6: LD C,C                         ; 49             I    
01c7: CPL                            ; 2f             /    
01c8: LD C,A                         ; 4f             O    
01c9: JR NZ,.+101                    ; 20 65          .e   
01cb: LD (HL),D                      ; 72             r    
01cc: LD (HL),D                      ; 72             r    
01cd: LD L,A                         ; 6f             o    
01ce: LD (HL),D                      ; 72             r    
01cf: NOP                            ; 00             .    
01d0: LD C,C                         ; 49             I    
01d1: LD L,(HL)                      ; 6e             n    
01d2: LD (HL),H                      ; 74             t    
01d3: LD H,L                         ; 65             e    
01d4: LD (HL),D                      ; 72             r    
01d5: LD (HL),D                      ; 72             r    
01d6: LD (HL),L                      ; 75             u    
01d7: LD (HL),B                      ; 70             p    
01d8: LD (HL),H                      ; 74             t    
01d9: LD H,L                         ; 65             e    
01da: LD H,H                         ; 64             d    
01db: JR NZ,.+115                    ; 20 73          .s   
01dd: LD A,C                         ; 79             y    
01de: LD (HL),E                      ; 73             s    
01df: LD (HL),H                      ; 74             t    
01e0: LD H,L                         ; 65             e    
01e1: LD L,L                         ; 6d             m    
01e2: JR NZ,.+99                     ; 20 63          .c   
01e4: LD H,C                         ; 61             a    
01e5: LD L,H                         ; 6c             l    
01e6: LD L,H                         ; 6c             l    
01e7: NOP                            ; 00             .    
01e8: LD C,(HL)                      ; 4e             N    
01e9: LD L,A                         ; 6f             o    
01ea: JR NZ,.+115                    ; 20 73          .s   
01ec: LD (HL),L                      ; 75             u    
01ed: LD H,E                         ; 63             c    
01ee: LD L,B                         ; 68             h    
01ef: JR NZ,.+112                    ; 20 70          .p   
01f1: LD (HL),D                      ; 72             r    
01f2: LD L,A                         ; 6f             o    
01f3: LD H,E                         ; 63             c    
01f4: LD H,L                         ; 65             e    
01f5: LD (HL),E                      ; 73             s    
01f6: LD (HL),E                      ; 73             s    
01f7: NOP                            ; 00             .    
01f8: LD C,(HL)                      ; 4e             N    
01f9: LD L,A                         ; 6f             o    
01fa: JR NZ,.+115                    ; 20 73          .s   
01fc: LD (HL),L                      ; 75             u    
01fd: LD H,E                         ; 63             c    
01fe: LD L,B                         ; 68             h    
01ff: JR NZ,.+102                    ; 20 66          .f   
0201: LD L,C                         ; 69             i    
0202: LD L,H                         ; 6c             l    
0203: LD H,L                         ; 65             e    
0204: JR NZ,.+111                    ; 20 6f          .o   
0206: LD (HL),D                      ; 72             r    
0207: JR NZ,.+100                    ; 20 64          .d   
0209: LD L,C                         ; 69             i    
020a: LD (HL),D                      ; 72             r    
020b: LD H,L                         ; 65             e    
020c: LD H,E                         ; 63             c    
020d: LD (HL),H                      ; 74             t    
020e: LD L,A                         ; 6f             o    
020f: LD (HL),D                      ; 72             r    
0210: LD A,C                         ; 79             y    
0211: NOP                            ; 00             .    
0212: LD C,(HL)                      ; 4e             N    
0213: LD L,A                         ; 6f             o    
0214: LD (HL),H                      ; 74             t    
0215: JR NZ,.+115                    ; 20 73          .s   
0217: LD (HL),L                      ; 75             u    
0218: LD (HL),B                      ; 70             p    
0219: LD H,L                         ; 65             e    
021a: LD (HL),D                      ; 72             r    
021b: DEC L                          ; 2d             -    
021c: LD (HL),L                      ; 75             u    
021d: LD (HL),E                      ; 73             s    
021e: LD H,L                         ; 65             e    
021f: LD (HL),D                      ; 72             r    
0220: NOP                            ; 00             .    
0221: LD D,L                         ; 55             U    
0222: LD L,(HL)                      ; 6e             n    
0223: LD L,E                         ; 6b             k    
0224: LD L,(HL)                      ; 6e             n    
0225: LD L,A                         ; 6f             o    
0226: LD (HL),A                      ; 77             w    
0227: LD L,(HL)                      ; 6e             n    
0228: JR NZ,.+101                    ; 20 65          .e   
022a: LD (HL),D                      ; 72             r    
022b: LD (HL),D                      ; 72             r    
022c: LD L,A                         ; 6f             o    
022d: LD (HL),D                      ; 72             r    
022e: NOP                            ; 00             .    
022f: LD A,(BC)                      ; 0a             .    
0230: NOP                            ; 00             .    
0231: LD A,(20)                      ; 3a 20 00       :..  
0234: CALL 0                         ; cd 00 00       ...  
0237: PUSH AF                        ; f5             .    
0238: PUSH AF                        ; f5             .    
0239: PUSH AF                        ; f5             .    
023a: PUSH AF                        ; f5             .    
023b: LD HL,4                        ; 21 04 00       !..  
023e: ADD HL,DE                      ; 19             .    
023f: LD A,(HL)                      ; 7e             ~    
0240: INC HL                         ; 23             #    
0241: OR A,(HL)                      ; b6             .    
0242: JP Z,283                       ; ca 83 02       ...  
0245: LD HL,4                        ; 21 04 00       !..  
0248: ADD HL,DE                      ; 19             .    
0249: LD A,(HL)                      ; 7e             ~    
024a: INC HL                         ; 23             #    
024b: LD H,(HL)                      ; 66             f    
024c: LD L,A                         ; 6f             o    
024d: LD A,(HL)                      ; 7e             ~    
024e: OR A,A                         ; b7             .    
024f: JP Z,283                       ; ca 83 02       ...  
0252: LD HL,4                        ; 21 04 00       !..  
0255: ADD HL,DE                      ; 19             .    
0256: LD C,(HL)                      ; 4e             N    
0257: INC HL                         ; 23             #    
0258: LD B,(HL)                      ; 46             F    
0259: PUSH BC                        ; c5             .    
025a: CALL 2fb                       ; cd fb 02       ...  
025d: POP AF                         ; f1             .    
025e: PUSH BC                        ; c5             .    
025f: LD HL,4                        ; 21 04 00       !..  
0262: ADD HL,DE                      ; 19             .    
0263: LD C,(HL)                      ; 4e             N    
0264: INC HL                         ; 23             #    
0265: LD B,(HL)                      ; 46             F    
0266: PUSH BC                        ; c5             .    
0267: LD HL,2                        ; 21 02 00       !..  
026a: PUSH HL                        ; e5             .    
026b: CALL 0                         ; cd 00 00       ...  
026e: POP AF                         ; f1             .    
026f: POP AF                         ; f1             .    
0270: POP AF                         ; f1             .    
0271: LD HL,2                        ; 21 02 00       !..  
0274: PUSH HL                        ; e5             .    
0275: LD HL,231                      ; 21 31 02       !1.  
0278: PUSH HL                        ; e5             .    
0279: LD HL,2                        ; 21 02 00       !..  
027c: PUSH HL                        ; e5             .    
027d: CALL 0                         ; cd 00 00       ...  
0280: POP AF                         ; f1             .    
0281: POP AF                         ; f1             .    
0282: POP AF                         ; f1             .    
0283: LD A,(0)                       ; 3a 00 00       :..  
0286: SUB A,                         ; d6 01          ..   
0288: LD A,(1)                       ; 3a 01 00       :..  
028b: SBC A,0                        ; de 00          ..   
028d: JP M,2ad                       ; fa ad 02       ...  
0290: LD HL,0                        ; 21 00 00       !..  
0293: LD A,                          ; 3e 20          >.   
0295: SUB A,(HL)                     ; 96             .    
0296: LD A,                          ; 3e 00          >.   
0298: INC HL                         ; 23             #    
0299: SBC A,(HL)                     ; 9e             .    
029a: JP M,2ad                       ; fa ad 02       ...  
029d: LD HL,fff8                     ; 21 f8 ff       !..  
02a0: ADD HL,DE                      ; 19             .    
02a1: LD A,(0)                       ; 3a 00 00       :..  
02a4: LD (HL),A                      ; 77             w    
02a5: LD A,(1)                       ; 3a 01 00       :..  
02a8: INC HL                         ; 23             #    
02a9: LD (HL),A                      ; 77             w    
02aa: JP 2b5                         ; c3 b5 02       ...  
02ad: LD HL,fff8                     ; 21 f8 ff       !..  
02b0: ADD HL,DE                      ; 19             .    
02b1: SUB A,A                        ; 97             .    
02b2: LD (HL),A                      ; 77             w    
02b3: INC HL                         ; 23             #    
02b4: LD (HL),A                      ; 77             w    
02b5: LD HL,fff8                     ; 21 f8 ff       !..  
02b8: ADD HL,DE                      ; 19             .    
02b9: LD A,(HL)                      ; 7e             ~    
02ba: INC HL                         ; 23             #    
02bb: LD H,(HL)                      ; 66             f    
02bc: LD L,A                         ; 6f             o    
02bd: ADD HL,HL                      ; 29             )    
02be: LD BC,333                      ; 01 33 03       .3.  
02c1: ADD HL,BC                      ; 09             .    
02c2: LD C,(HL)                      ; 4e             N    
02c3: INC HL                         ; 23             #    
02c4: LD B,(HL)                      ; 46             F    
02c5: PUSH BC                        ; c5             .    
02c6: CALL 2fb                       ; cd fb 02       ...  
02c9: POP AF                         ; f1             .    
02ca: PUSH BC                        ; c5             .    
02cb: LD HL,fff8                     ; 21 f8 ff       !..  
02ce: ADD HL,DE                      ; 19             .    
02cf: LD A,(HL)                      ; 7e             ~    
02d0: INC HL                         ; 23             #    
02d1: LD H,(HL)                      ; 66             f    
02d2: LD L,A                         ; 6f             o    
02d3: ADD HL,HL                      ; 29             )    
02d4: LD BC,333                      ; 01 33 03       .3.  
02d7: ADD HL,BC                      ; 09             .    
02d8: LD C,(HL)                      ; 4e             N    
02d9: INC HL                         ; 23             #    
02da: LD B,(HL)                      ; 46             F    
02db: PUSH BC                        ; c5             .    
02dc: LD HL,2                        ; 21 02 00       !..  
02df: PUSH HL                        ; e5             .    
02e0: CALL 0                         ; cd 00 00       ...  
02e3: POP AF                         ; f1             .    
02e4: POP AF                         ; f1             .    
02e5: POP AF                         ; f1             .    
02e6: LD HL,1                        ; 21 01 00       !..  
02e9: PUSH HL                        ; e5             .    
02ea: LD HL,22f                      ; 21 2f 02       !/.  
02ed: PUSH HL                        ; e5             .    
02ee: LD HL,2                        ; 21 02 00       !..  
02f1: PUSH HL                        ; e5             .    
02f2: CALL 0                         ; cd 00 00       ...  
02f5: POP AF                         ; f1             .    
02f6: POP AF                         ; f1             .    
02f7: POP AF                         ; f1             .    
02f8: JP 0                           ; c3 00 00       ...  
02fb: CALL 0                         ; cd 00 00       ...  
02fe: LD HL,4                        ; 21 04 00       !..  
0301: ADD HL,DE                      ; 19             .    
0302: LD A,(HL)                      ; 7e             ~    
0303: INC HL                         ; 23             #    
0304: LD H,(HL)                      ; 66             f    
0305: LD L,A                         ; 6f             o    
0306: LD (0),HL                      ; 22 00 00       "..  
0309: LD HL,(0)                      ; 2a 00 00       *..  
030c: LD (377),HL                    ; 22 77 03       "w.  
030f: LD HL,(377)                    ; 2a 77 03       *w.  
0312: LD A,(HL)                      ; 7e             ~    
0313: OR A,A                         ; b7             .    
0314: JP Z,321                       ; ca 21 03       .!.  
0317: LD HL,(377)                    ; 2a 77 03       *w.  
031a: INC HL                         ; 23             #    
031b: LD (377),HL                    ; 22 77 03       "w.  
031e: JP 30f                         ; c3 0f 03       ...  
0321: LD HL,(377)                    ; 2a 77 03       *w.  
0324: PUSH HL                        ; e5             .    
0325: LD HL,0                        ; 21 00 00       !..  
0328: POP BC                         ; c1             .    
0329: LD A,C                         ; 79             y    
032a: SUB A,(HL)                     ; 96             .    
032b: LD C,A                         ; 4f             O    
032c: LD A,B                         ; 78             x    
032d: INC HL                         ; 23             #    
032e: SBC A,(HL)                     ; 9e             .    
032f: LD B,A                         ; 47             G    
0330: JP 0                           ; c3 00 00       ...  
data:
0333: 21 02 12 02 f8 01 e8 01 d0 01 c6 01 ac 01 9a 01 
0343: 88 01 78 01 6c 01 5a 01 48 01 36 01 35 01 1f 01 
0353: 0b 01 ff 00 ed 00 de 00 ce 00 bf 00 ae 00 9a 00 
0363: 86 00 75 00 66 00 57 00 3f 00 32 00 1c 00 0d 00 
0373: 01 00 00 00 00 00 
../filesystem/lib/libu.a uname.o:
    0      c.r0: 0000 global 
    1   c.ulmod: 0000 global 
    2     c.ret: 0000 global 
    3   _getpid: 0000 global 
    4     c.ent: 0000 global 
    5     _ltob: 0000 global 
    6     _time: 0000 global 
    7     _itob: 0000 global 
    8   _cpystr: 0000 global 
    9    _uname: 0006 global defined code 
TEXT:00ad symbol reference c.ret
TEXT:00a8 data relative
TEXT:00a3 data relative
TEXT:00a0 data relative
TEXT:009c data relative
TEXT:0095 symbol reference _ltob
TEXT:0091 data relative
TEXT:0081 data relative
TEXT:0077 data relative
TEXT:0073 data relative
TEXT:006f data relative
TEXT:006c data relative
TEXT:0068 data relative
TEXT:0062 symbol reference _itob
TEXT:005e data relative
TEXT:005a symbol reference _getpid
TEXT:0053 data relative
TEXT:004b symbol reference _cpystr
TEXT:0047 data relative
TEXT:0043 text relative
TEXT:003b symbol reference c.ulmod
TEXT:0037 symbol reference c.r0
TEXT:0034 symbol reference c.r0
TEXT:002f symbol reference c.r0
TEXT:002a symbol reference c.r0
TEXT:0027 symbol reference c.r0
TEXT:001f data relative
TEXT:001b symbol reference _time
TEXT:0017 data relative
TEXT:0014 text relative
TEXT:000a data relative
TEXT:0007 symbol reference c.ent
0000: CPL                            ; 2f             /    
0001: LD (HL),H                      ; 74             t    
0002: LD L,L                         ; 6d             m    
0003: LD (HL),B                      ; 70             p    
0004: CPL                            ; 2f             /    
0005: NOP                            ; 00             .    
0006: CALL 0                         ; cd 00 00       ...  
0009: LD HL,c1                       ; 21 c1 00       !..  
000c: LD A,(HL)                      ; 7e             ~    
000d: INC HL                         ; 23             #    
000e: OR A,(HL)                      ; b6             .    
000f: INC HL                         ; 23             #    
0010: OR A,(HL)                      ; b6             .    
0011: INC HL                         ; 23             #    
0012: OR A,(HL)                      ; b6             .    
0013: JP NZ,3e                       ; c2 3e 00       .>.  
0016: LD HL,c1                       ; 21 c1 00       !..  
0019: PUSH HL                        ; e5             .    
001a: CALL 0                         ; cd 00 00       ...  
001d: POP AF                         ; f1             .    
001e: LD HL,c1                       ; 21 c1 00       !..  
0021: PUSH HL                        ; e5             .    
0022: LD A,                          ; 3e 27          >'   
0024: ADD A,A                        ; 87             .    
0025: SBC A,A                        ; 9f             .    
0026: LD (0),A                       ; 32 00 00       2..  
0029: LD (1),A                       ; 32 01 00       2..  
002c: LD A,                          ; 3e 27          >'   
002e: LD (3),A                       ; 32 03 00       2..  
0031: LD A,                          ; 3e 10          >.   
0033: LD (2),A                       ; 32 02 00       2..  
0036: LD HL,0                        ; 21 00 00       !..  
0039: PUSH HL                        ; e5             .    
003a: CALL 0                         ; cd 00 00       ...  
003d: POP AF                         ; f1             .    
003e: LD HL,0                        ; 21 00 00       !..  
0041: PUSH HL                        ; e5             .    
0042: LD HL,0                        ; 21 00 00       !..  
0045: PUSH HL                        ; e5             .    
0046: LD HL,af                       ; 21 af 00       !..  
0049: PUSH HL                        ; e5             .    
004a: CALL 0                         ; cd 00 00       ...  
004d: POP AF                         ; f1             .    
004e: POP AF                         ; f1             .    
004f: POP AF                         ; f1             .    
0050: LD L,C                         ; 69             i    
0051: LD H,B                         ; 60             `    
0052: LD (bf),HL                     ; 22 bf 00       "..  
0055: LD HL,a                        ; 21 0a 00       !..  
0058: PUSH HL                        ; e5             .    
0059: CALL 0                         ; cd 00 00       ...  
005c: PUSH BC                        ; c5             .    
005d: LD HL,(bf)                     ; 2a bf 00       *..  
0060: PUSH HL                        ; e5             .    
0061: CALL 0                         ; cd 00 00       ...  
0064: POP AF                         ; f1             .    
0065: POP AF                         ; f1             .    
0066: POP AF                         ; f1             .    
0067: LD HL,(bf)                     ; 2a bf 00       *..  
006a: ADD HL,BC                      ; 09             .    
006b: LD (bf),HL                     ; 22 bf 00       "..  
006e: LD HL,(bf)                     ; 2a bf 00       *..  
0071: PUSH HL                        ; e5             .    
0072: LD HL,(bf)                     ; 2a bf 00       *..  
0075: INC HL                         ; 23             #    
0076: LD (bf),HL                     ; 22 bf 00       "..  
0079: POP HL                         ; e1             .    
007a: LD (HL),                       ; 36 2d          6-   
007c: LD HL,a                        ; 21 0a 00       !..  
007f: PUSH HL                        ; e5             .    
0080: LD HL,c1                       ; 21 c1 00       !..  
0083: INC HL                         ; 23             #    
0084: INC HL                         ; 23             #    
0085: LD C,(HL)                      ; 4e             N    
0086: INC HL                         ; 23             #    
0087: LD B,(HL)                      ; 46             F    
0088: PUSH BC                        ; c5             .    
0089: DEC HL                         ; 2b             +    
008a: DEC HL                         ; 2b             +    
008b: DEC HL                         ; 2b             +    
008c: LD C,(HL)                      ; 4e             N    
008d: INC HL                         ; 23             #    
008e: LD B,(HL)                      ; 46             F    
008f: PUSH BC                        ; c5             .    
0090: LD HL,(bf)                     ; 2a bf 00       *..  
0093: PUSH HL                        ; e5             .    
0094: CALL 0                         ; cd 00 00       ...  
0097: POP AF                         ; f1             .    
0098: POP AF                         ; f1             .    
0099: POP AF                         ; f1             .    
009a: POP AF                         ; f1             .    
009b: LD HL,(bf)                     ; 2a bf 00       *..  
009e: ADD HL,BC                      ; 09             .    
009f: LD (bf),HL                     ; 22 bf 00       "..  
00a2: LD HL,(bf)                     ; 2a bf 00       *..  
00a5: LD (HL),                       ; 36 00          6.   
00a7: LD HL,af                       ; 21 af 00       !..  
00aa: LD C,L                         ; 4d             M    
00ab: LD B,H                         ; 44             D    
00ac: JP 0                           ; c3 00 00       ...  
data:
00af: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00bf: 00 00 00 00 00 00 
../filesystem/lib/libu.a create.o:
    0    _close: 0000 global 
    1    _creat: 0000 global 
    2     c.ret: 0000 global 
    3     c.ent: 0000 global 
    4     _open: 0000 global 
    5   _create: 0000 global defined code 
TEXT:00b3 symbol reference c.ret
TEXT:009e symbol reference _open
TEXT:0089 symbol reference _close
TEXT:007e text relative
TEXT:0077 text relative
TEXT:006d text relative
TEXT:0062 symbol reference _open
TEXT:004d symbol reference _close
TEXT:0042 text relative
TEXT:0038 symbol reference c.ret
TEXT:002b text relative
TEXT:0019 symbol reference _creat
TEXT:0001 symbol reference c.ent
0000: CALL 0                         ; cd 00 00       ...  
0003: PUSH AF                        ; f5             .    
0004: PUSH AF                        ; f5             .    
0005: PUSH AF                        ; f5             .    
0006: PUSH AF                        ; f5             .    
0007: LD HL,fff8                     ; 21 f8 ff       !..  
000a: ADD HL,DE                      ; 19             .    
000b: PUSH HL                        ; e5             .    
000c: LD HL,1ff                      ; 21 ff 01       !..  
000f: PUSH HL                        ; e5             .    
0010: LD HL,4                        ; 21 04 00       !..  
0013: ADD HL,DE                      ; 19             .    
0014: LD C,(HL)                      ; 4e             N    
0015: INC HL                         ; 23             #    
0016: LD B,(HL)                      ; 46             F    
0017: PUSH BC                        ; c5             .    
0018: CALL 0                         ; cd 00 00       ...  
001b: POP AF                         ; f1             .    
001c: POP AF                         ; f1             .    
001d: POP HL                         ; e1             .    
001e: LD A,C                         ; 79             y    
001f: LD (HL),A                      ; 77             w    
0020: LD A,B                         ; 78             x    
0021: INC HL                         ; 23             #    
0022: LD (HL),A                      ; 77             w    
0023: LD HL,fff8                     ; 21 f8 ff       !..  
0026: ADD HL,DE                      ; 19             .    
0027: INC HL                         ; 23             #    
0028: LD A,(HL)                      ; 7e             ~    
0029: OR A,A                         ; b7             .    
002a: JP P,3a                        ; f2 3a 00       .:.  
002d: LD HL,fff8                     ; 21 f8 ff       !..  
0030: ADD HL,DE                      ; 19             .    
0031: LD A,(HL)                      ; 7e             ~    
0032: INC HL                         ; 23             #    
0033: LD H,(HL)                      ; 66             f    
0034: LD L,A                         ; 6f             o    
0035: LD C,L                         ; 4d             M    
0036: LD B,H                         ; 44             D    
0037: JP 0                           ; c3 00 00       ...  
003a: LD HL,6                        ; 21 06 00       !..  
003d: ADD HL,DE                      ; 19             .    
003e: LD A,(HL)                      ; 7e             ~    
003f: INC HL                         ; 23             #    
0040: OR A,(HL)                      ; b6             .    
0041: JP NZ,6f                       ; c2 6f 00       .o.  
0044: LD HL,fff8                     ; 21 f8 ff       !..  
0047: ADD HL,DE                      ; 19             .    
0048: LD C,(HL)                      ; 4e             N    
0049: INC HL                         ; 23             #    
004a: LD B,(HL)                      ; 46             F    
004b: PUSH BC                        ; c5             .    
004c: CALL 0                         ; cd 00 00       ...  
004f: POP AF                         ; f1             .    
0050: LD HL,fff8                     ; 21 f8 ff       !..  
0053: ADD HL,DE                      ; 19             .    
0054: PUSH HL                        ; e5             .    
0055: LD HL,0                        ; 21 00 00       !..  
0058: PUSH HL                        ; e5             .    
0059: LD HL,4                        ; 21 04 00       !..  
005c: ADD HL,DE                      ; 19             .    
005d: LD C,(HL)                      ; 4e             N    
005e: INC HL                         ; 23             #    
005f: LD B,(HL)                      ; 46             F    
0060: PUSH BC                        ; c5             .    
0061: CALL 0                         ; cd 00 00       ...  
0064: POP AF                         ; f1             .    
0065: POP AF                         ; f1             .    
0066: POP HL                         ; e1             .    
0067: LD A,C                         ; 79             y    
0068: LD (HL),A                      ; 77             w    
0069: LD A,B                         ; 78             x    
006a: INC HL                         ; 23             #    
006b: LD (HL),A                      ; 77             w    
006c: JP a8                          ; c3 a8 00       ...  
006f: LD HL,6                        ; 21 06 00       !..  
0072: ADD HL,DE                      ; 19             .    
0073: LD A,(HL)                      ; 7e             ~    
0074: CP A,2                         ; fe 02          ..   
0076: JP NZ,7d                       ; c2 7d 00       .}.  
0079: INC HL                         ; 23             #    
007a: LD A,(HL)                      ; 7e             ~    
007b: CP A,0                         ; fe 00          ..   
007d: JP NZ,a8                       ; c2 a8 00       ...  
0080: LD HL,fff8                     ; 21 f8 ff       !..  
0083: ADD HL,DE                      ; 19             .    
0084: LD C,(HL)                      ; 4e             N    
0085: INC HL                         ; 23             #    
0086: LD B,(HL)                      ; 46             F    
0087: PUSH BC                        ; c5             .    
0088: CALL 0                         ; cd 00 00       ...  
008b: POP AF                         ; f1             .    
008c: LD HL,fff8                     ; 21 f8 ff       !..  
008f: ADD HL,DE                      ; 19             .    
0090: PUSH HL                        ; e5             .    
0091: LD HL,2                        ; 21 02 00       !..  
0094: PUSH HL                        ; e5             .    
0095: LD HL,4                        ; 21 04 00       !..  
0098: ADD HL,DE                      ; 19             .    
0099: LD C,(HL)                      ; 4e             N    
009a: INC HL                         ; 23             #    
009b: LD B,(HL)                      ; 46             F    
009c: PUSH BC                        ; c5             .    
009d: CALL 0                         ; cd 00 00       ...  
00a0: POP AF                         ; f1             .    
00a1: POP AF                         ; f1             .    
00a2: POP HL                         ; e1             .    
00a3: LD A,C                         ; 79             y    
00a4: LD (HL),A                      ; 77             w    
00a5: LD A,B                         ; 78             x    
00a6: INC HL                         ; 23             #    
00a7: LD (HL),A                      ; 77             w    
00a8: LD HL,fff8                     ; 21 f8 ff       !..  
00ab: ADD HL,DE                      ; 19             .    
00ac: LD A,(HL)                      ; 7e             ~    
00ad: INC HL                         ; 23             #    
00ae: LD H,(HL)                      ; 66             f    
00af: LD L,A                         ; 6f             o    
00b0: LD C,L                         ; 4d             M    
00b1: LD B,H                         ; 44             D    
00b2: JP 0                           ; c3 00 00       ...  
../filesystem/lib/libu.a execv.o:
    0     c.ret: 0000 global 
    1     c.ent: 0000 global 
    2     _exec: 0000 global 
    3    _execv: 0000 global defined code 
    4    _execl: 001b global defined code 
TEXT:0031 symbol reference c.ret
TEXT:002c symbol reference _exec
TEXT:001c symbol reference c.ent
TEXT:0019 symbol reference c.ret
TEXT:0014 symbol reference _exec
TEXT:0001 symbol reference c.ent
0000: CALL 0                         ; cd 00 00       ...  
0003: LD HL,6                        ; 21 06 00       !..  
0006: ADD HL,DE                      ; 19             .    
0007: LD C,(HL)                      ; 4e             N    
0008: INC HL                         ; 23             #    
0009: LD B,(HL)                      ; 46             F    
000a: PUSH BC                        ; c5             .    
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,DE                      ; 19             .    
000f: LD C,(HL)                      ; 4e             N    
0010: INC HL                         ; 23             #    
0011: LD B,(HL)                      ; 46             F    
0012: PUSH BC                        ; c5             .    
0013: CALL 0                         ; cd 00 00       ...  
0016: POP AF                         ; f1             .    
0017: POP AF                         ; f1             .    
0018: JP 0                           ; c3 00 00       ...  
001b: CALL 0                         ; cd 00 00       ...  
001e: LD HL,6                        ; 21 06 00       !..  
0021: ADD HL,DE                      ; 19             .    
0022: PUSH HL                        ; e5             .    
0023: LD HL,4                        ; 21 04 00       !..  
0026: ADD HL,DE                      ; 19             .    
0027: LD C,(HL)                      ; 4e             N    
0028: INC HL                         ; 23             #    
0029: LD B,(HL)                      ; 46             F    
002a: PUSH BC                        ; c5             .    
002b: CALL 0                         ; cd 00 00       ...  
002e: POP AF                         ; f1             .    
002f: POP AF                         ; f1             .    
0030: JP 0                           ; c3 00 00       ...  
../filesystem/lib/libu.a lseek.o:
    0    c.lmod: 0000 global 
    1      c.r1: 0000 global 
    2      c.r0: 0000 global 
    3     c.ret: 0000 global 
    4     c.ent: 0000 global 
    5    c.lcpy: 0000 global 
    6     _seek: 0000 global 
    7    c.ldiv: 0000 global 
    8    _lseek: 0000 global defined code 
TEXT:00f2 symbol reference c.ret
TEXT:00e5 symbol reference c.ret
TEXT:00df symbol reference c.ret
TEXT:00d9 text relative
TEXT:00d1 symbol reference _seek
TEXT:00b4 text relative
TEXT:00b1 text relative
TEXT:00a9 symbol reference _seek
TEXT:009d data relative
TEXT:0096 text relative
TEXT:008e symbol reference _seek
TEXT:0082 data relative
TEXT:0073 data relative
TEXT:006e data relative
TEXT:0067 symbol reference c.lmod
TEXT:0063 symbol reference c.r1
TEXT:0060 symbol reference c.r1
TEXT:005b symbol reference c.r1
TEXT:0058 symbol reference c.r1
TEXT:0055 symbol reference c.r1
TEXT:0050 symbol reference c.r0
TEXT:004d symbol reference c.lcpy
TEXT:004a symbol reference c.r0
TEXT:0041 data relative
TEXT:003c data relative
TEXT:0035 symbol reference c.ldiv
TEXT:0031 symbol reference c.r1
TEXT:002e symbol reference c.r1
TEXT:0029 symbol reference c.r1
TEXT:0026 symbol reference c.r1
TEXT:0023 symbol reference c.r1
TEXT:001e symbol reference c.r0
TEXT:001b symbol reference c.lcpy
TEXT:0018 symbol reference c.r0
TEXT:000f text relative
TEXT:0001 symbol reference c.ent
0000: CALL 0                         ; cd 00 00       ...  
0003: LD HL,a                        ; 21 0a 00       !..  
0006: ADD HL,DE                      ; 19             .    
0007: LD A,(HL)                      ; 7e             ~    
0008: SUB A,                         ; d6 03          ..   
000a: INC HL                         ; 23             #    
000b: LD A,(HL)                      ; 7e             ~    
000c: SBC A,0                        ; de 00          ..   
000e: JP P,b6                        ; f2 b6 00       ...  
0011: LD HL,6                        ; 21 06 00       !..  
0014: ADD HL,DE                      ; 19             .    
0015: PUSH HL                        ; e5             .    
0016: POP HL                         ; e1             .    
0017: LD BC,0                        ; 01 00 00       ...  
001a: CALL 0                         ; cd 00 00       ...  
001d: LD HL,0                        ; 21 00 00       !..  
0020: PUSH HL                        ; e5             .    
0021: SUB A,A                        ; 97             .    
0022: LD (0),A                       ; 32 00 00       2..  
0025: LD (1),A                       ; 32 01 00       2..  
0028: LD (2),A                       ; 32 02 00       2..  
002b: LD A,                          ; 3e 02          >.   
002d: LD (3),A                       ; 32 03 00       2..  
0030: LD HL,0                        ; 21 00 00       !..  
0033: PUSH HL                        ; e5             .    
0034: CALL 0                         ; cd 00 00       ...  
0037: POP HL                         ; e1             .    
0038: INC HL                         ; 23             #    
0039: INC HL                         ; 23             #    
003a: LD A,(HL)                      ; 7e             ~    
003b: LD (f4),A                      ; 32 f4 00       2..  
003e: INC HL                         ; 23             #    
003f: LD A,(HL)                      ; 7e             ~    
0040: LD (f5),A                      ; 32 f5 00       2..  
0043: LD HL,6                        ; 21 06 00       !..  
0046: ADD HL,DE                      ; 19             .    
0047: PUSH HL                        ; e5             .    
0048: POP HL                         ; e1             .    
0049: LD BC,0                        ; 01 00 00       ...  
004c: CALL 0                         ; cd 00 00       ...  
004f: LD HL,0                        ; 21 00 00       !..  
0052: PUSH HL                        ; e5             .    
0053: SUB A,A                        ; 97             .    
0054: LD (0),A                       ; 32 00 00       2..  
0057: LD (1),A                       ; 32 01 00       2..  
005a: LD (2),A                       ; 32 02 00       2..  
005d: LD A,                          ; 3e 02          >.   
005f: LD (3),A                       ; 32 03 00       2..  
0062: LD HL,0                        ; 21 00 00       !..  
0065: PUSH HL                        ; e5             .    
0066: CALL 0                         ; cd 00 00       ...  
0069: POP HL                         ; e1             .    
006a: INC HL                         ; 23             #    
006b: INC HL                         ; 23             #    
006c: LD A,(HL)                      ; 7e             ~    
006d: LD (f6),A                      ; 32 f6 00       2..  
0070: INC HL                         ; 23             #    
0071: LD A,(HL)                      ; 7e             ~    
0072: LD (f7),A                      ; 32 f7 00       2..  
0075: LD HL,a                        ; 21 0a 00       !..  
0078: ADD HL,DE                      ; 19             .    
0079: LD A,(HL)                      ; 7e             ~    
007a: INC HL                         ; 23             #    
007b: LD H,(HL)                      ; 66             f    
007c: LD L,A                         ; 6f             o    
007d: INC HL                         ; 23             #    
007e: INC HL                         ; 23             #    
007f: INC HL                         ; 23             #    
0080: PUSH HL                        ; e5             .    
0081: LD HL,(f4)                     ; 2a f4 00       *..  
0084: PUSH HL                        ; e5             .    
0085: LD HL,4                        ; 21 04 00       !..  
0088: ADD HL,DE                      ; 19             .    
0089: LD C,(HL)                      ; 4e             N    
008a: INC HL                         ; 23             #    
008b: LD B,(HL)                      ; 46             F    
008c: PUSH BC                        ; c5             .    
008d: CALL 0                         ; cd 00 00       ...  
0090: POP AF                         ; f1             .    
0091: POP AF                         ; f1             .    
0092: POP AF                         ; f1             .    
0093: LD A,B                         ; 78             x    
0094: OR A,A                         ; b7             .    
0095: JP M,e1                        ; fa e1 00       ...  
0098: LD HL,1                        ; 21 01 00       !..  
009b: PUSH HL                        ; e5             .    
009c: LD HL,(f6)                     ; 2a f6 00       *..  
009f: PUSH HL                        ; e5             .    
00a0: LD HL,4                        ; 21 04 00       !..  
00a3: ADD HL,DE                      ; 19             .    
00a4: LD C,(HL)                      ; 4e             N    
00a5: INC HL                         ; 23             #    
00a6: LD B,(HL)                      ; 46             F    
00a7: PUSH BC                        ; c5             .    
00a8: CALL 0                         ; cd 00 00       ...  
00ab: POP AF                         ; f1             .    
00ac: POP AF                         ; f1             .    
00ad: POP AF                         ; f1             .    
00ae: LD A,B                         ; 78             x    
00af: OR A,A                         ; b7             .    
00b0: JP P,e7                        ; f2 e7 00       ...  
00b3: JP e1                          ; c3 e1 00       ...  
00b6: LD HL,a                        ; 21 0a 00       !..  
00b9: ADD HL,DE                      ; 19             .    
00ba: LD C,(HL)                      ; 4e             N    
00bb: INC HL                         ; 23             #    
00bc: LD B,(HL)                      ; 46             F    
00bd: PUSH BC                        ; c5             .    
00be: LD HL,6                        ; 21 06 00       !..  
00c1: ADD HL,DE                      ; 19             .    
00c2: INC HL                         ; 23             #    
00c3: INC HL                         ; 23             #    
00c4: LD C,(HL)                      ; 4e             N    
00c5: INC HL                         ; 23             #    
00c6: LD B,(HL)                      ; 46             F    
00c7: PUSH BC                        ; c5             .    
00c8: LD HL,4                        ; 21 04 00       !..  
00cb: ADD HL,DE                      ; 19             .    
00cc: LD C,(HL)                      ; 4e             N    
00cd: INC HL                         ; 23             #    
00ce: LD B,(HL)                      ; 46             F    
00cf: PUSH BC                        ; c5             .    
00d0: CALL 0                         ; cd 00 00       ...  
00d3: POP AF                         ; f1             .    
00d4: POP AF                         ; f1             .    
00d5: POP AF                         ; f1             .    
00d6: LD A,B                         ; 78             x    
00d7: OR A,A                         ; b7             .    
00d8: JP P,e7                        ; f2 e7 00       ...  
00db: LD BC,ffff                     ; 01 ff ff       ...  
00de: JP 0                           ; c3 00 00       ...  
00e1: LD BC,ffff                     ; 01 ff ff       ...  
00e4: JP 0                           ; c3 00 00       ...  
00e7: LD HL,4                        ; 21 04 00       !..  
00ea: ADD HL,DE                      ; 19             .    
00eb: LD A,(HL)                      ; 7e             ~    
00ec: INC HL                         ; 23             #    
00ed: LD H,(HL)                      ; 66             f    
00ee: LD L,A                         ; 6f             o    
00ef: LD C,L                         ; 4d             M    
00f0: LD B,H                         ; 44             D    
00f1: JP 0                           ; c3 00 00       ...  
data:
00f4: 00 00 00 00 
../filesystem/lib/libu.a remove.o:
    0   _unlink: 0000 global 
    1     c.ret: 0000 global 
    2     c.ent: 0000 global 
    3   _remove: 0000 global defined code 
TEXT:0010 symbol reference c.ret
TEXT:000c symbol reference _unlink
TEXT:0001 symbol reference c.ent
0000: CALL 0                         ; cd 00 00       ...  
0003: LD HL,4                        ; 21 04 00       !..  
0006: ADD HL,DE                      ; 19             .    
0007: LD C,(HL)                      ; 4e             N    
0008: INC HL                         ; 23             #    
0009: LD B,(HL)                      ; 46             F    
000a: PUSH BC                        ; c5             .    
000b: CALL 0                         ; cd 00 00       ...  
000e: POP AF                         ; f1             .    
000f: JP 0                           ; c3 00 00       ...  
../filesystem/lib/libu.a sbreak.o:
    0     c.ret: 0000 global 
    1     c.ent: 0000 global 
    2  __memory: 0000 global 
    3   __break: 0000 global 
    4      _brk: 00ac global defined code 
    5   _sbreak: 0000 global defined code 
    6     _sbrk: 0042 global defined code 
DATA:0000 symbol reference __memory
TEXT:0105 symbol reference c.ret
TEXT:00f8 data relative
TEXT:00ed symbol reference c.ret
TEXT:00e7 text relative
TEXT:00e1 text relative
TEXT:00da symbol reference __break
TEXT:00bd data relative
TEXT:00b9 data relative
TEXT:00ad symbol reference c.ent
TEXT:00aa symbol reference c.ret
TEXT:009d data relative
TEXT:0092 symbol reference c.ret
TEXT:008c text relative
TEXT:0086 text relative
TEXT:007f symbol reference __break
TEXT:0053 data relative
TEXT:004f data relative
TEXT:0043 symbol reference c.ent
TEXT:0040 symbol reference c.ret
TEXT:0033 symbol reference c.ret
TEXT:002d text relative
TEXT:0026 text relative
TEXT:0015 text relative
TEXT:0001 symbol reference c.ent
0000: CALL 0                         ; cd 00 00       ...  
0003: PUSH AF                        ; f5             .    
0004: PUSH AF                        ; f5             .    
0005: PUSH AF                        ; f5             .    
0006: PUSH AF                        ; f5             .    
0007: LD HL,fff8                     ; 21 f8 ff       !..  
000a: ADD HL,DE                      ; 19             .    
000b: PUSH HL                        ; e5             .    
000c: LD HL,4                        ; 21 04 00       !..  
000f: ADD HL,DE                      ; 19             .    
0010: LD C,(HL)                      ; 4e             N    
0011: INC HL                         ; 23             #    
0012: LD B,(HL)                      ; 46             F    
0013: PUSH BC                        ; c5             .    
0014: CALL 42                        ; cd 42 00       .B.  
0017: POP AF                         ; f1             .    
0018: POP HL                         ; e1             .    
0019: LD A,C                         ; 79             y    
001a: LD (HL),A                      ; 77             w    
001b: LD A,B                         ; 78             x    
001c: INC HL                         ; 23             #    
001d: LD (HL),A                      ; 77             w    
001e: LD HL,fff8                     ; 21 f8 ff       !..  
0021: ADD HL,DE                      ; 19             .    
0022: LD A,(HL)                      ; 7e             ~    
0023: CP A,255                       ; fe ff          ..   
0025: JP NZ,2c                       ; c2 2c 00       .,.  
0028: INC HL                         ; 23             #    
0029: LD A,(HL)                      ; 7e             ~    
002a: CP A,255                       ; fe ff          ..   
002c: JP NZ,35                       ; c2 35 00       .5.  
002f: LD BC,0                        ; 01 00 00       ...  
0032: JP 0                           ; c3 00 00       ...  
0035: LD HL,fff8                     ; 21 f8 ff       !..  
0038: ADD HL,DE                      ; 19             .    
0039: LD A,(HL)                      ; 7e             ~    
003a: INC HL                         ; 23             #    
003b: LD H,(HL)                      ; 66             f    
003c: LD L,A                         ; 6f             o    
003d: LD C,L                         ; 4d             M    
003e: LD B,H                         ; 44             D    
003f: JP 0                           ; c3 00 00       ...  
0042: CALL 0                         ; cd 00 00       ...  
0045: LD HL,fff6                     ; 21 f6 ff       !..  
0048: ADD HL,SP                      ; 39             9    
0049: LD SP,HL                       ; f9             .    
004a: LD HL,fff8                     ; 21 f8 ff       !..  
004d: ADD HL,DE                      ; 19             .    
004e: LD A,(107)                     ; 3a 07 01       :..  
0051: LD (HL),A                      ; 77             w    
0052: LD A,(108)                     ; 3a 08 01       :..  
0055: INC HL                         ; 23             #    
0056: LD (HL),A                      ; 77             w    
0057: LD HL,fff6                     ; 21 f6 ff       !..  
005a: ADD HL,DE                      ; 19             .    
005b: PUSH HL                        ; e5             .    
005c: LD HL,fff8                     ; 21 f8 ff       !..  
005f: ADD HL,DE                      ; 19             .    
0060: LD A,(HL)                      ; 7e             ~    
0061: INC HL                         ; 23             #    
0062: LD H,(HL)                      ; 66             f    
0063: LD L,A                         ; 6f             o    
0064: PUSH HL                        ; e5             .    
0065: LD HL,4                        ; 21 04 00       !..  
0068: ADD HL,DE                      ; 19             .    
0069: LD A,(HL)                      ; 7e             ~    
006a: INC HL                         ; 23             #    
006b: LD H,(HL)                      ; 66             f    
006c: LD L,A                         ; 6f             o    
006d: EX (SP),HL                     ; e3             .    
006e: POP BC                         ; c1             .    
006f: ADD HL,BC                      ; 09             .    
0070: POP BC                         ; c1             .    
0071: LD A,L                         ; 7d             }    
0072: LD (BC),A                      ; 02             .    
0073: LD A,H                         ; 7c             |    
0074: INC BC                         ; 03             .    
0075: LD (BC),A                      ; 02             .    
0076: LD HL,fff6                     ; 21 f6 ff       !..  
0079: ADD HL,DE                      ; 19             .    
007a: LD C,(HL)                      ; 4e             N    
007b: INC HL                         ; 23             #    
007c: LD B,(HL)                      ; 46             F    
007d: PUSH BC                        ; c5             .    
007e: CALL 0                         ; cd 00 00       ...  
0081: POP AF                         ; f1             .    
0082: LD A,C                         ; 79             y    
0083: CP A,255                       ; fe ff          ..   
0085: JP NZ,8b                       ; c2 8b 00       ...  
0088: LD A,B                         ; 78             x    
0089: CP A,255                       ; fe ff          ..   
008b: JP NZ,94                       ; c2 94 00       ...  
008e: LD BC,ffff                     ; 01 ff ff       ...  
0091: JP 0                           ; c3 00 00       ...  
0094: LD HL,fff6                     ; 21 f6 ff       !..  
0097: ADD HL,DE                      ; 19             .    
0098: LD A,(HL)                      ; 7e             ~    
0099: INC HL                         ; 23             #    
009a: LD H,(HL)                      ; 66             f    
009b: LD L,A                         ; 6f             o    
009c: LD (107),HL                    ; 22 07 01       "..  
009f: LD HL,fff8                     ; 21 f8 ff       !..  
00a2: ADD HL,DE                      ; 19             .    
00a3: LD A,(HL)                      ; 7e             ~    
00a4: INC HL                         ; 23             #    
00a5: LD H,(HL)                      ; 66             f    
00a6: LD L,A                         ; 6f             o    
00a7: LD C,L                         ; 4d             M    
00a8: LD B,H                         ; 44             D    
00a9: JP 0                           ; c3 00 00       ...  
00ac: CALL 0                         ; cd 00 00       ...  
00af: LD HL,fff6                     ; 21 f6 ff       !..  
00b2: ADD HL,SP                      ; 39             9    
00b3: LD SP,HL                       ; f9             .    
00b4: LD HL,fff8                     ; 21 f8 ff       !..  
00b7: ADD HL,DE                      ; 19             .    
00b8: LD A,(107)                     ; 3a 07 01       :..  
00bb: LD (HL),A                      ; 77             w    
00bc: LD A,(108)                     ; 3a 08 01       :..  
00bf: INC HL                         ; 23             #    
00c0: LD (HL),A                      ; 77             w    
00c1: LD HL,fff6                     ; 21 f6 ff       !..  
00c4: ADD HL,DE                      ; 19             .    
00c5: PUSH HL                        ; e5             .    
00c6: LD HL,4                        ; 21 04 00       !..  
00c9: ADD HL,DE                      ; 19             .    
00ca: POP BC                         ; c1             .    
00cb: LD A,(HL)                      ; 7e             ~    
00cc: LD (BC),A                      ; 02             .    
00cd: INC HL                         ; 23             #    
00ce: LD A,(HL)                      ; 7e             ~    
00cf: INC BC                         ; 03             .    
00d0: LD (BC),A                      ; 02             .    
00d1: LD HL,fff6                     ; 21 f6 ff       !..  
00d4: ADD HL,DE                      ; 19             .    
00d5: LD C,(HL)                      ; 4e             N    
00d6: INC HL                         ; 23             #    
00d7: LD B,(HL)                      ; 46             F    
00d8: PUSH BC                        ; c5             .    
00d9: CALL 0                         ; cd 00 00       ...  
00dc: POP AF                         ; f1             .    
00dd: LD A,C                         ; 79             y    
00de: CP A,255                       ; fe ff          ..   
00e0: JP NZ,e6                       ; c2 e6 00       ...  
00e3: LD A,B                         ; 78             x    
00e4: CP A,255                       ; fe ff          ..   
00e6: JP NZ,ef                       ; c2 ef 00       ...  
00e9: LD BC,ffff                     ; 01 ff ff       ...  
00ec: JP 0                           ; c3 00 00       ...  
00ef: LD HL,fff6                     ; 21 f6 ff       !..  
00f2: ADD HL,DE                      ; 19             .    
00f3: LD A,(HL)                      ; 7e             ~    
00f4: INC HL                         ; 23             #    
00f5: LD H,(HL)                      ; 66             f    
00f6: LD L,A                         ; 6f             o    
00f7: LD (107),HL                    ; 22 07 01       "..  
00fa: LD HL,fff8                     ; 21 f8 ff       !..  
00fd: ADD HL,DE                      ; 19             .    
00fe: LD A,(HL)                      ; 7e             ~    
00ff: INC HL                         ; 23             #    
0100: LD H,(HL)                      ; 66             f    
0101: LD L,A                         ; 6f             o    
0102: LD C,L                         ; 4d             M    
0103: LD B,H                         ; 44             D    
0104: JP 0                           ; c3 00 00       ...  
data:
0107: 00 00 
../filesystem/lib/libu.a access.o:
    0    _errno: 0000 global 
    1   _access: 0000 global defined code 
TEXT:0020 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (25),HL                     ; 22 25 00       "%.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (27),HL                     ; 22 27 00       "'.  
0016: SYS INDIR                      ; cf 00 23 00    ..#. 
001a: LD BC,0                        ; 01 00 00       ...  
001d: RET NC                         ; d0             .    
001e: DEC BC                         ; 0b             .    
001f: LD (0),HL                      ; 22 00 00       "..  
0022: RET                            ; c9             .    
data:
0023: cf 21 00 00 00 00 
../filesystem/lib/libu.a alarm.o:
    0    _errno: 0000 global 
    1    _alarm: 0000 global defined code 
TEXT:000a data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: SYS INDIR                      ; cf 00 10 00    .... 
000c: LD BC,0                        ; 01 00 00       ...  
000f: RET                            ; c9             .    
data:
0010: cf 1b 
../filesystem/lib/libu.a break.o:
    0    _errno: 0000 global 
    1   __break: 0000 global defined code 
TEXT:0011 symbol reference _errno
TEXT:0009 data relative
TEXT:0005 data relative
0000: POP BC                         ; c1             .    
0001: POP HL                         ; e1             .    
0002: PUSH HL                        ; e5             .    
0003: PUSH BC                        ; c5             .    
0004: LD (16),HL                     ; 22 16 00       "..  
0007: SYS INDIR                      ; cf 00 14 00    .... 
000b: LD BC,0                        ; 01 00 00       ...  
000e: RET NC                         ; d0             .    
000f: DEC BC                         ; 0b             .    
0010: LD (0),HL                      ; 22 00 00       "..  
0013: RET                            ; c9             .    
data:
0014: cf 11 00 00 
../filesystem/lib/libu.a chdir.o:
    0    _errno: 0000 global 
    1    _chdir: 0000 global defined code 
TEXT:0015 symbol reference _errno
TEXT:000d data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (1a),HL                     ; 22 1a 00       "..  
000b: SYS INDIR                      ; cf 00 18 00    .... 
000f: LD BC,0                        ; 01 00 00       ...  
0012: RET NC                         ; d0             .    
0013: DEC BC                         ; 0b             .    
0014: LD (0),HL                      ; 22 00 00       "..  
0017: RET                            ; c9             .    
data:
0018: cf 0c 00 00 
../filesystem/lib/libu.a chmod.o:
    0    _errno: 0000 global 
    1    _chmod: 0000 global defined code 
TEXT:0020 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (25),HL                     ; 22 25 00       "%.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (27),HL                     ; 22 27 00       "'.  
0016: SYS INDIR                      ; cf 00 23 00    ..#. 
001a: LD BC,0                        ; 01 00 00       ...  
001d: RET NC                         ; d0             .    
001e: DEC BC                         ; 0b             .    
001f: LD (0),HL                      ; 22 00 00       "..  
0022: RET                            ; c9             .    
data:
0023: cf 0f 00 00 00 00 
../filesystem/lib/libu.a chown.o:
    0    _errno: 0000 global 
    1    _chown: 0000 global defined code 
TEXT:0020 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (25),HL                     ; 22 25 00       "%.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (27),HL                     ; 22 27 00       "'.  
0016: SYS INDIR                      ; cf 00 23 00    ..#. 
001a: LD BC,0                        ; 01 00 00       ...  
001d: RET NC                         ; d0             .    
001e: DEC BC                         ; 0b             .    
001f: LD (0),HL                      ; 22 00 00       "..  
0022: RET                            ; c9             .    
data:
0023: cf 10 00 00 00 00 
../filesystem/lib/libu.a close.o:
    0    _errno: 0000 global 
    1    _close: 0000 global defined code 
TEXT:000c symbol reference _errno
0000: POP BC                         ; c1             .    
0001: POP HL                         ; e1             .    
0002: PUSH HL                        ; e5             .    
0003: PUSH BC                        ; c5             .    
0004: SYS CLOSE                      ; cf 06          ..   
0006: LD BC,0                        ; 01 00 00       ...  
0009: RET NC                         ; d0             .    
000a: DEC BC                         ; 0b             .    
000b: LD (0),HL                      ; 22 00 00       "..  
000e: RET                            ; c9             .    
../filesystem/lib/libu.a creat.o:
    0    _errno: 0000 global 
    1    _creat: 0000 global defined code 
TEXT:0021 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (26),HL                     ; 22 26 00       "&.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (28),HL                     ; 22 28 00       "(.  
0016: SYS INDIR                      ; cf 00 24 00    ..$. 
001a: LD C,L                         ; 4d             M    
001b: LD B,H                         ; 44             D    
001c: RET NC                         ; d0             .    
001d: LD BC,ffff                     ; 01 ff ff       ...  
0020: LD (0),HL                      ; 22 00 00       "..  
0023: RET                            ; c9             .    
data:
0024: cf 08 00 00 00 00 
../filesystem/lib/libu.a dup.o:
    0    _errno: 0000 global 
    1      _dup: 0000 global defined code 
TEXT:0011 symbol reference _errno
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: SYS DUP                        ; cf 29          .)   
000a: LD C,L                         ; 4d             M    
000b: LD B,H                         ; 44             D    
000c: RET NC                         ; d0             .    
000d: LD BC,ffff                     ; 01 ff ff       ...  
0010: LD (0),HL                      ; 22 00 00       "..  
0013: RET                            ; c9             .    
../filesystem/lib/libu.a exec.o:
    0    _errno: 0000 global 
    1     _exec: 0000 global defined code 
TEXT:001e symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (23),HL                     ; 22 23 00       "#.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (25),HL                     ; 22 25 00       "%.  
0016: SYS INDIR                      ; cf 00 21 00    ..!. 
001a: LD BC,ffff                     ; 01 ff ff       ...  
001d: LD (0),HL                      ; 22 00 00       "..  
0020: RET                            ; c9             .    
data:
0021: cf 0b 00 00 00 00 
../filesystem/lib/libu.a exit.o:
    0    __exit: 0000 global 
    1     c.ret: 0000 global 
    2    c.enô: 0000 global 
    3     _exit: 0000 global defined code 
TEXT:0010 symbol reference c.ret
TEXT:000c symbol reference __exit
TEXT:0001 symbol reference c.enô
0000: CALL 0                         ; cd 00 00       ...  
0003: LD HL,4                        ; 21 04 00       !..  
0006: ADD HL,DE                      ; 19             .    
0007: LD C,(HL)                      ; 4e             N    
0008: INC HL                         ; 23             #    
0009: LD B,(HL)                      ; 46             F    
000a: PUSH BC                        ; c5             .    
000b: CALL 0                         ; cd 00 00       ...  
000e: POP AF                         ; f1             .    
000f: JP 0                           ; c3 00 00       ...  
../filesystem/lib/libu.a _exit.o:
    0    _errno: 0000 global 
    1    __exit: 0000 global defined code 
0000: POP BC                         ; c1             .    
0001: POP HL                         ; e1             .    
0002: PUSH HL                        ; e5             .    
0003: PUSH BC                        ; c5             .    
0004: SYS EXIT                       ; cf 01          ..   
../filesystem/lib/libu.a fork.o:
    0    _errno: 0000 global 
    1     _fork: 0000 global defined code 
TEXT:000c symbol reference _errno
TEXT:0003 text relative
0000: SYS FORK                       ; cf 02          ..   
0002: JP f                           ; c3 0f 00       ...  
0005: LD C,L                         ; 4d             M    
0006: LD B,H                         ; 44             D    
0007: RET NC                         ; d0             .    
0008: LD BC,ffff                     ; 01 ff ff       ...  
000b: LD (0),HL                      ; 22 00 00       "..  
000e: RET                            ; c9             .    
000f: LD BC,0                        ; 01 00 00       ...  
0012: RET                            ; c9             .    
../filesystem/lib/libu.a fstat.o:
    0    _errno: 0000 global 
    1    _fstat: 0000 global defined code 
TEXT:001d symbol reference _errno
TEXT:0015 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (22),HL                     ; 22 22 00       "".  
000b: LD HL,2                        ; 21 02 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: SYS INDIR                      ; cf 00 20 00    .... 
0017: LD BC,0                        ; 01 00 00       ...  
001a: RET NC                         ; d0             .    
001b: DEC BC                         ; 0b             .    
001c: LD (0),HL                      ; 22 00 00       "..  
001f: RET                            ; c9             .    
data:
0020: cf 1c 00 00 
../filesystem/lib/libu.a getpid.o:
    0    _errno: 0000 global 
    1   _getpid: 0000 global defined code 
0000: SYS GETPID                     ; cf 14          ..   
0002: LD C,L                         ; 4d             M    
0003: LD B,H                         ; 44             D    
0004: RET                            ; c9             .    
../filesystem/lib/libu.a getuid.o:
    0    _errno: 0000 global 
    1   _getuid: 0000 global defined code 
0000: SYS GETUID                     ; cf 18          ..   
0002: LD C,L                         ; 4d             M    
0003: LD B,H                         ; 44             D    
0004: RET                            ; c9             .    
../filesystem/lib/libu.a gtty.o:
    0    _errno: 0000 global 
    1     _gtty: 0000 global defined code 
TEXT:001d symbol reference _errno
TEXT:0015 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (22),HL                     ; 22 22 00       "".  
000b: LD HL,2                        ; 21 02 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: SYS INDIR                      ; cf 00 20 00    .... 
0017: LD BC,0                        ; 01 00 00       ...  
001a: RET NC                         ; d0             .    
001b: DEC BC                         ; 0b             .    
001c: LD (0),HL                      ; 22 00 00       "..  
001f: RET                            ; c9             .    
data:
0020: cf 20 00 00 
../filesystem/lib/libu.a kill.o:
    0    _errno: 0000 global 
    1     _kill: 0000 global defined code 
TEXT:001d symbol reference _errno
TEXT:0015 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (22),HL                     ; 22 22 00       "".  
000b: LD HL,2                        ; 21 02 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: SYS INDIR                      ; cf 00 20 00    .... 
0017: LD BC,0                        ; 01 00 00       ...  
001a: RET NC                         ; d0             .    
001b: DEC BC                         ; 0b             .    
001c: LD (0),HL                      ; 22 00 00       "..  
001f: RET                            ; c9             .    
data:
0020: cf 25 00 00 
../filesystem/lib/libu.a link.o:
    0    _errno: 0000 global 
    1     _link: 0000 global defined code 
TEXT:0020 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (25),HL                     ; 22 25 00       "%.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (27),HL                     ; 22 27 00       "'.  
0016: SYS INDIR                      ; cf 00 23 00    ..#. 
001a: LD BC,0                        ; 01 00 00       ...  
001d: RET NC                         ; d0             .    
001e: DEC BC                         ; 0b             .    
001f: LD (0),HL                      ; 22 00 00       "..  
0022: RET                            ; c9             .    
data:
0023: cf 09 00 00 00 00 
../filesystem/lib/libu.a mknod.o:
    0    _errno: 0000 global 
    1    _mknod: 0000 global defined code 
TEXT:002b symbol reference _errno
TEXT:0023 data relative
TEXT:001f data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (30),HL                     ; 22 30 00       "0.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (32),HL                     ; 22 32 00       "2.  
0016: LD HL,6                        ; 21 06 00       !..  
0019: ADD HL,SP                      ; 39             9    
001a: LD A,(HL)                      ; 7e             ~    
001b: INC HL                         ; 23             #    
001c: LD H,(HL)                      ; 66             f    
001d: LD L,A                         ; 6f             o    
001e: LD (34),HL                     ; 22 34 00       "4.  
0021: SYS INDIR                      ; cf 00 2e 00    .... 
0025: LD BC,0                        ; 01 00 00       ...  
0028: RET NC                         ; d0             .    
0029: DEC BC                         ; 0b             .    
002a: LD (0),HL                      ; 22 00 00       "..  
002d: RET                            ; c9             .    
data:
002e: cf 0e 00 00 00 00 00 00 
../filesystem/lib/libu.a mount.o:
    0    _errno: 0000 global 
    1    _mount: 0000 global defined code 
TEXT:002b symbol reference _errno
TEXT:0023 data relative
TEXT:001f data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (30),HL                     ; 22 30 00       "0.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (32),HL                     ; 22 32 00       "2.  
0016: LD HL,6                        ; 21 06 00       !..  
0019: ADD HL,SP                      ; 39             9    
001a: LD A,(HL)                      ; 7e             ~    
001b: INC HL                         ; 23             #    
001c: LD H,(HL)                      ; 66             f    
001d: LD L,A                         ; 6f             o    
001e: LD (34),HL                     ; 22 34 00       "4.  
0021: SYS INDIR                      ; cf 00 2e 00    .... 
0025: LD BC,0                        ; 01 00 00       ...  
0028: RET NC                         ; d0             .    
0029: DEC BC                         ; 0b             .    
002a: LD (0),HL                      ; 22 00 00       "..  
002d: RET                            ; c9             .    
data:
002e: cf 15 00 00 00 00 00 00 
../filesystem/lib/libu.a nice.o:
    0    _errno: 0000 global 
    1     _nice: 0000 global defined code 
TEXT:0010 symbol reference _errno
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: SYS NICE                       ; cf 22          ."   
000a: LD BC,0                        ; 01 00 00       ...  
000d: RET NC                         ; d0             .    
000e: DEC BC                         ; 0b             .    
000f: LD (0),HL                      ; 22 00 00       "..  
0012: RET                            ; c9             .    
../filesystem/lib/libu.a open.o:
    0    _errno: 0000 global 
    1     _open: 0000 global defined code 
TEXT:0021 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (26),HL                     ; 22 26 00       "&.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (28),HL                     ; 22 28 00       "(.  
0016: SYS INDIR                      ; cf 00 24 00    ..$. 
001a: LD C,L                         ; 4d             M    
001b: LD B,H                         ; 44             D    
001c: RET NC                         ; d0             .    
001d: LD BC,ffff                     ; 01 ff ff       ...  
0020: LD (0),HL                      ; 22 00 00       "..  
0023: RET                            ; c9             .    
data:
0024: cf 05 00 00 00 00 
../filesystem/lib/libu.a pause.o:
    0    _errno: 0000 global 
    1    _pause: 0000 global defined code 
TEXT:0002 data relative
0000: SYS INDIR                      ; cf 00 08 00    .... 
0004: LD BC,0                        ; 01 00 00       ...  
0007: RET                            ; c9             .    
data:
0008: cf 1d 
../filesystem/lib/libu.a pipe.o:
    0    _errno: 0000 global 
    1     _pipe: 0000 global defined code 
TEXT:001c symbol reference _errno
0000: PUSH DE                        ; d5             .    
0001: SYS PIPE                       ; cf 2a          .*   
0003: EX DE,HL                       ; eb             .    
0004: PUSH HL                        ; e5             .    
0005: LD HL,6                        ; 21 06 00       !..  
0008: ADD HL,SP                      ; 39             9    
0009: LD A,(HL)                      ; 7e             ~    
000a: INC HL                         ; 23             #    
000b: LD H,(HL)                      ; 66             f    
000c: LD L,A                         ; 6f             o    
000d: LD (HL),E                      ; 73             s    
000e: INC HL                         ; 23             #    
000f: LD (HL),D                      ; 72             r    
0010: INC HL                         ; 23             #    
0011: POP DE                         ; d1             .    
0012: LD (HL),E                      ; 73             s    
0013: INC HL                         ; 23             #    
0014: LD (HL),D                      ; 72             r    
0015: POP DE                         ; d1             .    
0016: LD BC,0                        ; 01 00 00       ...  
0019: RET NC                         ; d0             .    
001a: DEC BC                         ; 0b             .    
001b: LD (0),HL                      ; 22 00 00       "..  
001e: RET                            ; c9             .    
../filesystem/lib/libu.a read.o:
    0    _errno: 0000 global 
    1     _read: 0000 global defined code 
TEXT:0029 symbol reference _errno
TEXT:0020 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (2e),HL                     ; 22 2e 00       "..  
000b: LD HL,6                        ; 21 06 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (30),HL                     ; 22 30 00       "0.  
0016: LD HL,2                        ; 21 02 00       !..  
0019: ADD HL,SP                      ; 39             9    
001a: LD A,(HL)                      ; 7e             ~    
001b: INC HL                         ; 23             #    
001c: LD H,(HL)                      ; 66             f    
001d: LD L,A                         ; 6f             o    
001e: SYS INDIR                      ; cf 00 2c 00    ..,. 
0022: LD C,L                         ; 4d             M    
0023: LD B,H                         ; 44             D    
0024: RET NC                         ; d0             .    
0025: LD BC,ffff                     ; 01 ff ff       ...  
0028: LD (0),HL                      ; 22 00 00       "..  
002b: RET                            ; c9             .    
data:
002c: cf 03 00 00 00 00 
../filesystem/lib/libu.a seek.o:
    0    _errno: 0000 global 
    1     _seek: 0000 global defined code 
TEXT:0028 symbol reference _errno
TEXT:0020 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (2d),HL                     ; 22 2d 00       "-.  
000b: LD HL,6                        ; 21 06 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (2f),HL                     ; 22 2f 00       "/.  
0016: LD HL,2                        ; 21 02 00       !..  
0019: ADD HL,SP                      ; 39             9    
001a: LD A,(HL)                      ; 7e             ~    
001b: INC HL                         ; 23             #    
001c: LD H,(HL)                      ; 66             f    
001d: LD L,A                         ; 6f             o    
001e: SYS INDIR                      ; cf 00 2b 00    ..+. 
0022: LD BC,0                        ; 01 00 00       ...  
0025: RET NC                         ; d0             .    
0026: DEC BC                         ; 0b             .    
0027: LD (0),HL                      ; 22 00 00       "..  
002a: RET                            ; c9             .    
data:
002b: cf 13 00 00 00 00 
../filesystem/lib/libu.a setuid.o:
    0    _errno: 0000 global 
    1   _setuid: 0000 global defined code 
TEXT:0010 symbol reference _errno
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: SYS SETUID                     ; cf 17          ..   
000a: LD BC,0                        ; 01 00 00       ...  
000d: RET NC                         ; d0             .    
000e: DEC BC                         ; 0b             .    
000f: LD (0),HL                      ; 22 00 00       "..  
0012: RET                            ; c9             .    
../filesystem/lib/libu.a signal.o:
    0  __signal: 0000 global 
    1      c.r4: 0000 global 
    2      c.r2: 0000 global 
    3    __stab: 0000 global 
    4    __jtab: 0000 global 
    5    c.rets: 0000 global 
    6    c.ents: 0000 global 
    7   _signal: 0000 global defined code 
TEXT:00e9 symbol reference c.rets
TEXT:00e4 data relative
TEXT:00df symbol reference __signal
TEXT:00db symbol reference c.r4
TEXT:00d6 symbol reference __jtab
TEXT:00c7 symbol reference c.r4
TEXT:00c2 symbol reference c.r2
TEXT:00be symbol reference c.r2
TEXT:00ba symbol reference __stab
TEXT:00b2 symbol reference c.r4
TEXT:00af text relative
TEXT:00a9 symbol reference c.r2
TEXT:00a6 text relative
TEXT:00a1 symbol reference c.r2
TEXT:009e text relative
TEXT:0099 symbol reference c.r2
TEXT:0096 data relative
TEXT:008e symbol reference __stab
TEXT:0086 symbol reference c.r4
TEXT:0083 text relative
TEXT:007d data relative
TEXT:007a text relative
TEXT:0075 data relative
TEXT:0072 text relative
TEXT:006d data relative
TEXT:006a data relative
TEXT:0063 symbol reference __signal
TEXT:005f symbol reference c.r4
TEXT:0059 symbol reference c.r2
TEXT:0056 text relative
TEXT:0050 text relative
TEXT:004a symbol reference c.r2
TEXT:0047 text relative
TEXT:0042 symbol reference c.r2
TEXT:003f text relative
TEXT:003a symbol reference c.r2
TEXT:0037 symbol reference c.rets
TEXT:0031 text relative
TEXT:0027 symbol reference c.r4
TEXT:0024 text relative
TEXT:001f symbol reference c.r4
TEXT:001a symbol reference c.r4
TEXT:0017 symbol reference c.r2
TEXT:000c symbol reference c.r4
TEXT:0001 symbol reference c.ents
0000: CALL 0                         ; cd 00 00       ...  
0003: LD HL,4                        ; 21 04 00       !..  
0006: ADD HL,DE                      ; 19             .    
0007: LD A,(HL)                      ; 7e             ~    
0008: INC HL                         ; 23             #    
0009: LD H,(HL)                      ; 66             f    
000a: LD L,A                         ; 6f             o    
000b: LD (0),HL                      ; 22 00 00       "..  
000e: LD HL,6                        ; 21 06 00       !..  
0011: ADD HL,DE                      ; 19             .    
0012: LD A,(HL)                      ; 7e             ~    
0013: INC HL                         ; 23             #    
0014: LD H,(HL)                      ; 66             f    
0015: LD L,A                         ; 6f             o    
0016: LD (0),HL                      ; 22 00 00       "..  
0019: LD A,(0)                       ; 3a 00 00       :..  
001c: SUB A,                         ; d6 01          ..   
001e: LD A,(1)                       ; 3a 01 00       :..  
0021: SBC A,0                        ; de 00          ..   
0023: JP M,33                        ; fa 33 00       .3.  
0026: LD HL,0                        ; 21 00 00       !..  
0029: LD A,                          ; 3e 0f          >.   
002b: SUB A,(HL)                     ; 96             .    
002c: LD A,                          ; 3e 00          >.   
002e: INC HL                         ; 23             #    
002f: SBC A,(HL)                     ; 9e             .    
0030: JP P,39                        ; f2 39 00       .9.  
0033: LD BC,ffff                     ; 01 ff ff       ...  
0036: JP 0                           ; c3 00 00       ...  
0039: LD A,(0)                       ; 3a 00 00       :..  
003c: CP A,1                         ; fe 01          ..   
003e: JP NZ,46                       ; c2 46 00       .F.  
0041: LD A,(1)                       ; 3a 01 00       :..  
0044: CP A,0                         ; fe 00          ..   
0046: JP Z,58                        ; ca 58 00       .X.  
0049: LD HL,0                        ; 21 00 00       !..  
004c: LD A,(HL)                      ; 7e             ~    
004d: INC HL                         ; 23             #    
004e: OR A,(HL)                      ; b6             .    
004f: JP Z,58                        ; ca 58 00       .X.  
0052: LD BC,1                        ; 01 01 00       ...  
0055: JP 5d                          ; c3 5d 00       .].  
0058: LD HL,(0)                      ; 2a 00 00       *..  
005b: LD C,L                         ; 4d             M    
005c: LD B,H                         ; 44             D    
005d: PUSH BC                        ; c5             .    
005e: LD HL,(0)                      ; 2a 00 00       *..  
0061: PUSH HL                        ; e5             .    
0062: CALL 0                         ; cd 00 00       ...  
0065: POP AF                         ; f1             .    
0066: POP AF                         ; f1             .    
0067: LD L,C                         ; 69             i    
0068: LD H,B                         ; 60             `    
0069: LD (eb),HL                     ; 22 eb 00       "..  
006c: LD A,(eb)                      ; 3a eb 00       :..  
006f: CP A,1                         ; fe 01          ..   
0071: JP NZ,79                       ; c2 79 00       .y.  
0074: LD A,(ec)                      ; 3a ec 00       :..  
0077: CP A,0                         ; fe 00          ..   
0079: JP Z,98                        ; ca 98 00       ...  
007c: LD HL,eb                       ; 21 eb 00       !..  
007f: LD A,(HL)                      ; 7e             ~    
0080: INC HL                         ; 23             #    
0081: OR A,(HL)                      ; b6             .    
0082: JP Z,98                        ; ca 98 00       ...  
0085: LD HL,(0)                      ; 2a 00 00       *..  
0088: LD BC,ffff                     ; 01 ff ff       ...  
008b: ADD HL,BC                      ; 09             .    
008c: ADD HL,HL                      ; 29             )    
008d: LD BC,0                        ; 01 00 00       ...  
0090: ADD HL,BC                      ; 09             .    
0091: LD A,(HL)                      ; 7e             ~    
0092: INC HL                         ; 23             #    
0093: LD H,(HL)                      ; 66             f    
0094: LD L,A                         ; 6f             o    
0095: LD (eb),HL                     ; 22 eb 00       "..  
0098: LD A,(0)                       ; 3a 00 00       :..  
009b: CP A,1                         ; fe 01          ..   
009d: JP NZ,a5                       ; c2 a5 00       ...  
00a0: LD A,(1)                       ; 3a 01 00       :..  
00a3: CP A,0                         ; fe 00          ..   
00a5: JP Z,e3                        ; ca e3 00       ...  
00a8: LD HL,0                        ; 21 00 00       !..  
00ab: LD A,(HL)                      ; 7e             ~    
00ac: INC HL                         ; 23             #    
00ad: OR A,(HL)                      ; b6             .    
00ae: JP Z,e3                        ; ca e3 00       ...  
00b1: LD HL,(0)                      ; 2a 00 00       *..  
00b4: LD BC,ffff                     ; 01 ff ff       ...  
00b7: ADD HL,BC                      ; 09             .    
00b8: ADD HL,HL                      ; 29             )    
00b9: LD BC,0                        ; 01 00 00       ...  
00bc: ADD HL,BC                      ; 09             .    
00bd: LD A,(0)                       ; 3a 00 00       :..  
00c0: LD (HL),A                      ; 77             w    
00c1: LD A,(1)                       ; 3a 01 00       :..  
00c4: INC HL                         ; 23             #    
00c5: LD (HL),A                      ; 77             w    
00c6: LD HL,(0)                      ; 2a 00 00       *..  
00c9: LD BC,ffff                     ; 01 ff ff       ...  
00cc: ADD HL,BC                      ; 09             .    
00cd: PUSH BC                        ; c5             .    
00ce: LD C,L                         ; 4d             M    
00cf: LD B,H                         ; 44             D    
00d0: ADD HL,HL                      ; 29             )    
00d1: ADD HL,BC                      ; 09             .    
00d2: ADD HL,HL                      ; 29             )    
00d3: ADD HL,BC                      ; 09             .    
00d4: POP BC                         ; c1             .    
00d5: LD BC,0                        ; 01 00 00       ...  
00d8: ADD HL,BC                      ; 09             .    
00d9: PUSH HL                        ; e5             .    
00da: LD HL,(0)                      ; 2a 00 00       *..  
00dd: PUSH HL                        ; e5             .    
00de: CALL 0                         ; cd 00 00       ...  
00e1: POP AF                         ; f1             .    
00e2: POP AF                         ; f1             .    
00e3: LD HL,(eb)                     ; 2a eb 00       *..  
00e6: LD C,L                         ; 4d             M    
00e7: LD B,H                         ; 44             D    
00e8: JP 0                           ; c3 00 00       ...  
data:
00eb: 00 00 
../filesystem/lib/libu.a _signal.o:
    0     c.ihl: 0000 global 
    1    _errno: 0000 global 
    2  __signal: 0000 global defined code 
    3    __stab: 009e global defined data 
    4    __jtab: 0024 global defined code 
TEXT:0091 symbol reference c.ihl
TEXT:008b text relative
TEXT:0088 data relative
TEXT:0084 text relative
TEXT:0081 data relative
TEXT:007d text relative
TEXT:007a data relative
TEXT:0076 text relative
TEXT:0073 data relative
TEXT:006f text relative
TEXT:006c data relative
TEXT:0068 text relative
TEXT:0065 data relative
TEXT:0061 text relative
TEXT:005e data relative
TEXT:005a text relative
TEXT:0057 data relative
TEXT:0053 text relative
TEXT:0050 data relative
TEXT:004c text relative
TEXT:0049 data relative
TEXT:0045 text relative
TEXT:0042 data relative
TEXT:003e text relative
TEXT:003b data relative
TEXT:0037 text relative
TEXT:0034 data relative
TEXT:0030 text relative
TEXT:002d data relative
TEXT:0029 text relative
TEXT:0026 data relative
TEXT:0021 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (9a),HL                     ; 22 9a 00       "..  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (9c),HL                     ; 22 9c 00       "..  
0016: SYS INDIR                      ; cf 00 98 00    .... 
001a: LD C,L                         ; 4d             M    
001b: LD B,H                         ; 44             D    
001c: RET NC                         ; d0             .    
001d: LD BC,ffff                     ; 01 ff ff       ...  
0020: LD (0),HL                      ; 22 00 00       "..  
0023: RET                            ; c9             .    
0024: PUSH HL                        ; e5             .    
0025: LD HL,(9e)                     ; 2a 9e 00       *..  
0028: JP 8d                          ; c3 8d 00       ...  
002b: PUSH HL                        ; e5             .    
002c: LD HL,(a0)                     ; 2a a0 00       *..  
002f: JP 8d                          ; c3 8d 00       ...  
0032: PUSH HL                        ; e5             .    
0033: LD HL,(a2)                     ; 2a a2 00       *..  
0036: JP 8d                          ; c3 8d 00       ...  
0039: PUSH HL                        ; e5             .    
003a: LD HL,(a4)                     ; 2a a4 00       *..  
003d: JP 8d                          ; c3 8d 00       ...  
0040: PUSH HL                        ; e5             .    
0041: LD HL,(a6)                     ; 2a a6 00       *..  
0044: JP 8d                          ; c3 8d 00       ...  
0047: PUSH HL                        ; e5             .    
0048: LD HL,(a8)                     ; 2a a8 00       *..  
004b: JP 8d                          ; c3 8d 00       ...  
004e: PUSH HL                        ; e5             .    
004f: LD HL,(aa)                     ; 2a aa 00       *..  
0052: JP 8d                          ; c3 8d 00       ...  
0055: PUSH HL                        ; e5             .    
0056: LD HL,(ac)                     ; 2a ac 00       *..  
0059: JP 8d                          ; c3 8d 00       ...  
005c: PUSH HL                        ; e5             .    
005d: LD HL,(ae)                     ; 2a ae 00       *..  
0060: JP 8d                          ; c3 8d 00       ...  
0063: PUSH HL                        ; e5             .    
0064: LD HL,(b0)                     ; 2a b0 00       *..  
0067: JP 8d                          ; c3 8d 00       ...  
006a: PUSH HL                        ; e5             .    
006b: LD HL,(b2)                     ; 2a b2 00       *..  
006e: JP 8d                          ; c3 8d 00       ...  
0071: PUSH HL                        ; e5             .    
0072: LD HL,(b4)                     ; 2a b4 00       *..  
0075: JP 8d                          ; c3 8d 00       ...  
0078: PUSH HL                        ; e5             .    
0079: LD HL,(b6)                     ; 2a b6 00       *..  
007c: JP 8d                          ; c3 8d 00       ...  
007f: PUSH HL                        ; e5             .    
0080: LD HL,(b8)                     ; 2a b8 00       *..  
0083: JP 8d                          ; c3 8d 00       ...  
0086: PUSH HL                        ; e5             .    
0087: LD HL,(ba)                     ; 2a ba 00       *..  
008a: JP 8d                          ; c3 8d 00       ...  
008d: PUSH DE                        ; d5             .    
008e: PUSH BC                        ; c5             .    
008f: PUSH AF                        ; f5             .    
0090: CALL 0                         ; cd 00 00       ...  
0093: POP AF                         ; f1             .    
0094: POP BC                         ; c1             .    
0095: POP DE                         ; d1             .    
0096: POP HL                         ; e1             .    
0097: RET                            ; c9             .    
data:
0098: cf 30 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00a8: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00b8: 00 00 00 00 
../filesystem/lib/libu.a sleep.o:
    0    _errno: 0000 global 
    1    _sleep: 0000 global defined code 
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: SYS SLEEP                      ; cf 23          .#   
000a: LD BC,0                        ; 01 00 00       ...  
000d: RET                            ; c9             .    
../filesystem/lib/libu.a stat.o:
    0    _errno: 0000 global 
    1     _stat: 0000 global defined code 
TEXT:0020 symbol reference _errno
TEXT:0018 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (25),HL                     ; 22 25 00       "%.  
000b: LD HL,4                        ; 21 04 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (27),HL                     ; 22 27 00       "'.  
0016: SYS INDIR                      ; cf 00 23 00    ..#. 
001a: LD BC,0                        ; 01 00 00       ...  
001d: RET NC                         ; d0             .    
001e: DEC BC                         ; 0b             .    
001f: LD (0),HL                      ; 22 00 00       "..  
0022: RET                            ; c9             .    
data:
0023: cf 12 00 00 00 00 
../filesystem/lib/libu.a stime.o:
    0    _errno: 0000 global 
    1    _stime: 0000 global defined code 
TEXT:001b symbol reference _errno
0000: PUSH DE                        ; d5             .    
0001: LD HL,4                        ; 21 04 00       !..  
0004: ADD HL,SP                      ; 39             9    
0005: LD A,(HL)                      ; 7e             ~    
0006: INC HL                         ; 23             #    
0007: LD H,(HL)                      ; 66             f    
0008: LD L,A                         ; 6f             o    
0009: LD E,(HL)                      ; 5e             ^    
000a: INC HL                         ; 23             #    
000b: LD D,(HL)                      ; 56             V    
000c: INC HL                         ; 23             #    
000d: LD A,(HL)                      ; 7e             ~    
000e: INC HL                         ; 23             #    
000f: LD H,(HL)                      ; 66             f    
0010: LD L,A                         ; 6f             o    
0011: EX DE,HL                       ; eb             .    
0012: SYS STIME                      ; cf 19          ..   
0014: POP DE                         ; d1             .    
0015: LD BC,0                        ; 01 00 00       ...  
0018: RET NC                         ; d0             .    
0019: DEC BC                         ; 0b             .    
001a: LD (0),HL                      ; 22 00 00       "..  
001d: RET                            ; c9             .    
../filesystem/lib/libu.a stty.o:
    0    _errno: 0000 global 
    1     _stty: 0000 global defined code 
TEXT:001d symbol reference _errno
TEXT:0015 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (22),HL                     ; 22 22 00       "".  
000b: LD HL,2                        ; 21 02 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: SYS INDIR                      ; cf 00 20 00    .... 
0017: LD BC,0                        ; 01 00 00       ...  
001a: RET NC                         ; d0             .    
001b: DEC BC                         ; 0b             .    
001c: LD (0),HL                      ; 22 00 00       "..  
001f: RET                            ; c9             .    
data:
0020: cf 1f 00 00 
../filesystem/lib/libu.a sync.o:
    0    _errno: 0000 global 
    1     _sync: 0000 global defined code 
0000: SYS SYNC                       ; cf 24          .$   
0002: LD BC,0                        ; 01 00 00       ...  
0005: RET                            ; c9             .    
../filesystem/lib/libu.a time.o:
    0    _errno: 0000 global 
    1     _time: 0000 global defined code 
TEXT:001c symbol reference _errno
0000: PUSH DE                        ; d5             .    
0001: SYS TIME                       ; cf 0d          ..   
0003: EX DE,HL                       ; eb             .    
0004: PUSH HL                        ; e5             .    
0005: LD HL,6                        ; 21 06 00       !..  
0008: ADD HL,SP                      ; 39             9    
0009: LD A,(HL)                      ; 7e             ~    
000a: INC HL                         ; 23             #    
000b: LD H,(HL)                      ; 66             f    
000c: LD L,A                         ; 6f             o    
000d: LD (HL),E                      ; 73             s    
000e: INC HL                         ; 23             #    
000f: LD (HL),D                      ; 72             r    
0010: INC HL                         ; 23             #    
0011: POP DE                         ; d1             .    
0012: LD (HL),E                      ; 73             s    
0013: INC HL                         ; 23             #    
0014: LD (HL),D                      ; 72             r    
0015: POP DE                         ; d1             .    
0016: LD BC,0                        ; 01 00 00       ...  
0019: RET NC                         ; d0             .    
001a: DEC BC                         ; 0b             .    
001b: LD (0),HL                      ; 22 00 00       "..  
001e: RET                            ; c9             .    
../filesystem/lib/libu.a umount.o:
    0    _errno: 0000 global 
    1   _umount: 0000 global defined code 
TEXT:0015 symbol reference _errno
TEXT:000d data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (1a),HL                     ; 22 1a 00       "..  
000b: SYS INDIR                      ; cf 00 18 00    .... 
000f: LD BC,0                        ; 01 00 00       ...  
0012: RET NC                         ; d0             .    
0013: DEC BC                         ; 0b             .    
0014: LD (0),HL                      ; 22 00 00       "..  
0017: RET                            ; c9             .    
data:
0018: cf 16 00 00 
../filesystem/lib/libu.a unlink.o:
    0    _errno: 0000 global 
    1   _unlink: 0000 global defined code 
TEXT:0015 symbol reference _errno
TEXT:000d data relative
TEXT:0009 data relative
0000: LD HL,2                        ; 21 02 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (1a),HL                     ; 22 1a 00       "..  
000b: SYS INDIR                      ; cf 00 18 00    .... 
000f: LD BC,0                        ; 01 00 00       ...  
0012: RET NC                         ; d0             .    
0013: DEC BC                         ; 0b             .    
0014: LD (0),HL                      ; 22 00 00       "..  
0017: RET                            ; c9             .    
data:
0018: cf 0a 00 00 
../filesystem/lib/libu.a wait.o:
    0    _errno: 0000 global 
    1     _wait: 0000 global defined code 
TEXT:001a symbol reference _errno
TEXT:0004 text relative
0000: PUSH DE                        ; d5             .    
0001: SYS WAIT                       ; cf 07          ..   
0003: JP C,15                        ; da 15 00       ...  
0006: LD C,L                         ; 4d             M    
0007: LD B,H                         ; 44             D    
0008: LD HL,4                        ; 21 04 00       !..  
000b: ADD HL,SP                      ; 39             9    
000c: LD A,(HL)                      ; 7e             ~    
000d: INC HL                         ; 23             #    
000e: LD H,(HL)                      ; 66             f    
000f: LD L,A                         ; 6f             o    
0010: LD (HL),E                      ; 73             s    
0011: INC HL                         ; 23             #    
0012: LD (HL),D                      ; 72             r    
0013: POP DE                         ; d1             .    
0014: RET                            ; c9             .    
0015: POP DE                         ; d1             .    
0016: LD BC,ffff                     ; 01 ff ff       ...  
0019: LD (0),HL                      ; 22 00 00       "..  
001c: RET                            ; c9             .    
../filesystem/lib/libu.a write.o:
    0    _errno: 0000 global 
    1    _write: 0000 global defined code 
TEXT:0029 symbol reference _errno
TEXT:0020 data relative
TEXT:0014 data relative
TEXT:0009 data relative
0000: LD HL,4                        ; 21 04 00       !..  
0003: ADD HL,SP                      ; 39             9    
0004: LD A,(HL)                      ; 7e             ~    
0005: INC HL                         ; 23             #    
0006: LD H,(HL)                      ; 66             f    
0007: LD L,A                         ; 6f             o    
0008: LD (2e),HL                     ; 22 2e 00       "..  
000b: LD HL,6                        ; 21 06 00       !..  
000e: ADD HL,SP                      ; 39             9    
000f: LD A,(HL)                      ; 7e             ~    
0010: INC HL                         ; 23             #    
0011: LD H,(HL)                      ; 66             f    
0012: LD L,A                         ; 6f             o    
0013: LD (30),HL                     ; 22 30 00       "0.  
0016: LD HL,2                        ; 21 02 00       !..  
0019: ADD HL,SP                      ; 39             9    
001a: LD A,(HL)                      ; 7e             ~    
001b: INC HL                         ; 23             #    
001c: LD H,(HL)                      ; 66             f    
001d: LD L,A                         ; 6f             o    
001e: SYS INDIR                      ; cf 00 2c 00    ..,. 
0022: LD C,L                         ; 4d             M    
0023: LD B,H                         ; 44             D    
0024: RET NC                         ; d0             .    
0025: LD BC,ffff                     ; 01 ff ff       ...  
0028: LD (0),HL                      ; 22 00 00       "..  
002b: RET                            ; c9             .    
data:
002c: cf 04 00 00 00 00 
../filesystem/lib/libu.a errno.o:
    0    _errno: 0000 global defined data 
data:
0000: 00 00 
