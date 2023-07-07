;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

sbreak.o:
    0     c.ret: 0000 08 global 
    1     c.ent: 0000 08 global 
    2  __memory: 0000 08 global 
    3   __break: 0000 08 global 
    4      _brk: 00ac 0d global defined code 
    5   _sbreak: 0000 0d global defined code 
    6     _sbrk: 0042 0d global defined code 
0000: call 0x0                       ; cd 00 00       ...  
0003: push af                        ; f5             .    
0004: push af                        ; f5             .    
0005: push af                        ; f5             .    
0006: push af                        ; f5             .    
0007: ld hl,0xfff8                   ; 21 f8 ff       !..  
000a: add hl,de                      ; 19             .    
000b: push hl                        ; e5             .    
000c: ld hl,0x4                      ; 21 04 00       !..  
000f: add hl,de                      ; 19             .    
0010: ld c,(hl)                      ; 4e             n    
0011: inc hl                         ; 23             #    
0012: ld b,(hl)                      ; 46             f    
0013: push bc                        ; c5             .    
0014: call 0x42                      ; cd 42 00       .b.  
0017: pop af                         ; f1             .    
0018: pop hl                         ; e1             .    
0019: ld a,c                         ; 79             y    
001a: ld (hl),a                      ; 77             w    
001b: ld a,b                         ; 78             x    
001c: inc hl                         ; 23             #    
001d: ld (hl),a                      ; 77             w    
001e: ld hl,0xfff8                   ; 21 f8 ff       !..  
0021: add hl,de                      ; 19             .    
0022: ld a,(hl)                      ; 7e             ~    
0023: cp a,0xff                      ; fe ff          ..   
0025: jp nz,0x2c                     ; c2 2c 00       .,.  
0028: inc hl                         ; 23             #    
0029: ld a,(hl)                      ; 7e             ~    
002a: cp a,0xff                      ; fe ff          ..   
002c: jp nz,0x35                     ; c2 35 00       .5.  
002f: ld bc,0x0                      ; 01 00 00       ...  
0032: jp 0x0                         ; c3 00 00       ...  
0035: ld hl,0xfff8                   ; 21 f8 ff       !..  
0038: add hl,de                      ; 19             .    
0039: ld a,(hl)                      ; 7e             ~    
003a: inc hl                         ; 23             #    
003b: ld h,(hl)                      ; 66             f    
003c: ld l,a                         ; 6f             o    
003d: ld c,l                         ; 4d             m    
003e: ld b,h                         ; 44             d    
003f: jp 0x0                         ; c3 00 00       ...  
0042: call 0x0                       ; cd 00 00       ...  
0045: ld hl,0xfff6                   ; 21 f6 ff       !..  
0048: add hl,sp                      ; 39             9    
0049: ld sp,hl                       ; f9             .    
004a: ld hl,0xfff8                   ; 21 f8 ff       !..  
004d: add hl,de                      ; 19             .    
004e: ld a,(0x107)                   ; 3a 07 01       :..  
0051: ld (hl),a                      ; 77             w    
0052: ld a,(0x108)                   ; 3a 08 01       :..  
0055: inc hl                         ; 23             #    
0056: ld (hl),a                      ; 77             w    
0057: ld hl,0xfff6                   ; 21 f6 ff       !..  
005a: add hl,de                      ; 19             .    
005b: push hl                        ; e5             .    
005c: ld hl,0xfff8                   ; 21 f8 ff       !..  
005f: add hl,de                      ; 19             .    
0060: ld a,(hl)                      ; 7e             ~    
0061: inc hl                         ; 23             #    
0062: ld h,(hl)                      ; 66             f    
0063: ld l,a                         ; 6f             o    
0064: push hl                        ; e5             .    
0065: ld hl,0x4                      ; 21 04 00       !..  
0068: add hl,de                      ; 19             .    
0069: ld a,(hl)                      ; 7e             ~    
006a: inc hl                         ; 23             #    
006b: ld h,(hl)                      ; 66             f    
006c: ld l,a                         ; 6f             o    
006d: ex (sp),hl                     ; e3             .    
006e: pop bc                         ; c1             .    
006f: add hl,bc                      ; 09             .    
0070: pop bc                         ; c1             .    
0071: ld a,l                         ; 7d             }    
0072: ld (bc),a                      ; 02             .    
0073: ld a,h                         ; 7c             |    
0074: inc bc                         ; 03             .    
0075: ld (bc),a                      ; 02             .    
0076: ld hl,0xfff6                   ; 21 f6 ff       !..  
0079: add hl,de                      ; 19             .    
007a: ld c,(hl)                      ; 4e             n    
007b: inc hl                         ; 23             #    
007c: ld b,(hl)                      ; 46             f    
007d: push bc                        ; c5             .    
007e: call 0x0                       ; cd 00 00       ...  
0081: pop af                         ; f1             .    
0082: ld a,c                         ; 79             y    
0083: cp a,0xff                      ; fe ff          ..   
0085: jp nz,0x8b                     ; c2 8b 00       ...  
0088: ld a,b                         ; 78             x    
0089: cp a,0xff                      ; fe ff          ..   
008b: jp nz,0x94                     ; c2 94 00       ...  
008e: ld bc,0xffff                   ; 01 ff ff       ...  
0091: jp 0x0                         ; c3 00 00       ...  
0094: ld hl,0xfff6                   ; 21 f6 ff       !..  
0097: add hl,de                      ; 19             .    
0098: ld a,(hl)                      ; 7e             ~    
0099: inc hl                         ; 23             #    
009a: ld h,(hl)                      ; 66             f    
009b: ld l,a                         ; 6f             o    
009c: ld (0x107),hl                  ; 22 07 01       "..  
009f: ld hl,0xfff8                   ; 21 f8 ff       !..  
00a2: add hl,de                      ; 19             .    
00a3: ld a,(hl)                      ; 7e             ~    
00a4: inc hl                         ; 23             #    
00a5: ld h,(hl)                      ; 66             f    
00a6: ld l,a                         ; 6f             o    
00a7: ld c,l                         ; 4d             m    
00a8: ld b,h                         ; 44             d    
00a9: jp 0x0                         ; c3 00 00       ...  
00ac: call 0x0                       ; cd 00 00       ...  
00af: ld hl,0xfff6                   ; 21 f6 ff       !..  
00b2: add hl,sp                      ; 39             9    
00b3: ld sp,hl                       ; f9             .    
00b4: ld hl,0xfff8                   ; 21 f8 ff       !..  
00b7: add hl,de                      ; 19             .    
00b8: ld a,(0x107)                   ; 3a 07 01       :..  
00bb: ld (hl),a                      ; 77             w    
00bc: ld a,(0x108)                   ; 3a 08 01       :..  
00bf: inc hl                         ; 23             #    
00c0: ld (hl),a                      ; 77             w    
00c1: ld hl,0xfff6                   ; 21 f6 ff       !..  
00c4: add hl,de                      ; 19             .    
00c5: push hl                        ; e5             .    
00c6: ld hl,0x4                      ; 21 04 00       !..  
00c9: add hl,de                      ; 19             .    
00ca: pop bc                         ; c1             .    
00cb: ld a,(hl)                      ; 7e             ~    
00cc: ld (bc),a                      ; 02             .    
00cd: inc hl                         ; 23             #    
00ce: ld a,(hl)                      ; 7e             ~    
00cf: inc bc                         ; 03             .    
00d0: ld (bc),a                      ; 02             .    
00d1: ld hl,0xfff6                   ; 21 f6 ff       !..  
00d4: add hl,de                      ; 19             .    
00d5: ld c,(hl)                      ; 4e             n    
00d6: inc hl                         ; 23             #    
00d7: ld b,(hl)                      ; 46             f    
00d8: push bc                        ; c5             .    
00d9: call 0x0                       ; cd 00 00       ...  
00dc: pop af                         ; f1             .    
00dd: ld a,c                         ; 79             y    
00de: cp a,0xff                      ; fe ff          ..   
00e0: jp nz,0xe6                     ; c2 e6 00       ...  
00e3: ld a,b                         ; 78             x    
00e4: cp a,0xff                      ; fe ff          ..   
00e6: jp nz,0xef                     ; c2 ef 00       ...  
00e9: ld bc,0xffff                   ; 01 ff ff       ...  
00ec: jp 0x0                         ; c3 00 00       ...  
00ef: ld hl,0xfff6                   ; 21 f6 ff       !..  
00f2: add hl,de                      ; 19             .    
00f3: ld a,(hl)                      ; 7e             ~    
00f4: inc hl                         ; 23             #    
00f5: ld h,(hl)                      ; 66             f    
00f6: ld l,a                         ; 6f             o    
00f7: ld (0x107),hl                  ; 22 07 01       "..  
00fa: ld hl,0xfff8                   ; 21 f8 ff       !..  
00fd: add hl,de                      ; 19             .    
00fe: ld a,(hl)                      ; 7e             ~    
00ff: inc hl                         ; 23             #    
0100: ld h,(hl)                      ; 66             f    
0101: ld l,a                         ; 6f             o    
0102: ld c,l                         ; 4d             m    
0103: ld b,h                         ; 44             d    
0104: jp 0x0                         ; c3 00 00       ...  
data:
0107: 00 00                                           ..
