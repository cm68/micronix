;
; assembly source for lseek system call
;
; /usr/src/lib/libu/lseek.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

lseek.o:
    0    c.lmod: 0000 08 global 
    1      c.r1: 0000 08 global 
    2      c.r0: 0000 08 global 
    3     c.ret: 0000 08 global 
    4     c.ent: 0000 08 global 
    5    c.lcpy: 0000 08 global 
    6     _seek: 0000 08 global 
    7    c.ldiv: 0000 08 global 
    8    _lseek: 0000 0d global defined code 
0000: call 0x0                       ; cd 00 00       ...  
0003: ld hl,0xa                      ; 21 0a 00       !..  
0006: add hl,de                      ; 19             .    
0007: ld a,(hl)                      ; 7e             ~    
0008: sub a,0x3                      ; d6 03          ..   
000a: inc hl                         ; 23             #    
000b: ld a,(hl)                      ; 7e             ~    
000c: sbc a,0x0                      ; de 00          ..   
000e: jp p,0xb6                      ; f2 b6 00       ...  
0011: ld hl,0x6                      ; 21 06 00       !..  
0014: add hl,de                      ; 19             .    
0015: push hl                        ; e5             .    
0016: pop hl                         ; e1             .    
0017: ld bc,0x0                      ; 01 00 00       ...  
001a: call 0x0                       ; cd 00 00       ...  
001d: ld hl,0x0                      ; 21 00 00       !..  
0020: push hl                        ; e5             .    
0021: sub a,a                        ; 97             .    
0022: ld (0x0),a                     ; 32 00 00       2..  
0025: ld (0x1),a                     ; 32 01 00       2..  
0028: ld (0x2),a                     ; 32 02 00       2..  
002b: ld a,0x2                       ; 3e 02          >.   
002d: ld (0x3),a                     ; 32 03 00       2..  
0030: ld hl,0x0                      ; 21 00 00       !..  
0033: push hl                        ; e5             .    
0034: call 0x0                       ; cd 00 00       ...  
0037: pop hl                         ; e1             .    
0038: inc hl                         ; 23             #    
0039: inc hl                         ; 23             #    
003a: ld a,(hl)                      ; 7e             ~    
003b: ld (0xf4),a                    ; 32 f4 00       2..  
003e: inc hl                         ; 23             #    
003f: ld a,(hl)                      ; 7e             ~    
0040: ld (0xf5),a                    ; 32 f5 00       2..  
0043: ld hl,0x6                      ; 21 06 00       !..  
0046: add hl,de                      ; 19             .    
0047: push hl                        ; e5             .    
0048: pop hl                         ; e1             .    
0049: ld bc,0x0                      ; 01 00 00       ...  
004c: call 0x0                       ; cd 00 00       ...  
004f: ld hl,0x0                      ; 21 00 00       !..  
0052: push hl                        ; e5             .    
0053: sub a,a                        ; 97             .    
0054: ld (0x0),a                     ; 32 00 00       2..  
0057: ld (0x1),a                     ; 32 01 00       2..  
005a: ld (0x2),a                     ; 32 02 00       2..  
005d: ld a,0x2                       ; 3e 02          >.   
005f: ld (0x3),a                     ; 32 03 00       2..  
0062: ld hl,0x0                      ; 21 00 00       !..  
0065: push hl                        ; e5             .    
0066: call 0x0                       ; cd 00 00       ...  
0069: pop hl                         ; e1             .    
006a: inc hl                         ; 23             #    
006b: inc hl                         ; 23             #    
006c: ld a,(hl)                      ; 7e             ~    
006d: ld (0xf6),a                    ; 32 f6 00       2..  
0070: inc hl                         ; 23             #    
0071: ld a,(hl)                      ; 7e             ~    
0072: ld (0xf7),a                    ; 32 f7 00       2..  
0075: ld hl,0xa                      ; 21 0a 00       !..  
0078: add hl,de                      ; 19             .    
0079: ld a,(hl)                      ; 7e             ~    
007a: inc hl                         ; 23             #    
007b: ld h,(hl)                      ; 66             f    
007c: ld l,a                         ; 6f             o    
007d: inc hl                         ; 23             #    
007e: inc hl                         ; 23             #    
007f: inc hl                         ; 23             #    
0080: push hl                        ; e5             .    
0081: ld hl,(0xf4)                   ; 2a f4 00       *..  
0084: push hl                        ; e5             .    
0085: ld hl,0x4                      ; 21 04 00       !..  
0088: add hl,de                      ; 19             .    
0089: ld c,(hl)                      ; 4e             n    
008a: inc hl                         ; 23             #    
008b: ld b,(hl)                      ; 46             f    
008c: push bc                        ; c5             .    
008d: call 0x0                       ; cd 00 00       ...  
0090: pop af                         ; f1             .    
0091: pop af                         ; f1             .    
0092: pop af                         ; f1             .    
0093: ld a,b                         ; 78             x    
0094: or a,a                         ; b7             .    
0095: jp m,0xe1                      ; fa e1 00       ...  
0098: ld hl,0x1                      ; 21 01 00       !..  
009b: push hl                        ; e5             .    
009c: ld hl,(0xf6)                   ; 2a f6 00       *..  
009f: push hl                        ; e5             .    
00a0: ld hl,0x4                      ; 21 04 00       !..  
00a3: add hl,de                      ; 19             .    
00a4: ld c,(hl)                      ; 4e             n    
00a5: inc hl                         ; 23             #    
00a6: ld b,(hl)                      ; 46             f    
00a7: push bc                        ; c5             .    
00a8: call 0x0                       ; cd 00 00       ...  
00ab: pop af                         ; f1             .    
00ac: pop af                         ; f1             .    
00ad: pop af                         ; f1             .    
00ae: ld a,b                         ; 78             x    
00af: or a,a                         ; b7             .    
00b0: jp p,0xe7                      ; f2 e7 00       ...  
00b3: jp 0xe1                        ; c3 e1 00       ...  
00b6: ld hl,0xa                      ; 21 0a 00       !..  
00b9: add hl,de                      ; 19             .    
00ba: ld c,(hl)                      ; 4e             n    
00bb: inc hl                         ; 23             #    
00bc: ld b,(hl)                      ; 46             f    
00bd: push bc                        ; c5             .    
00be: ld hl,0x6                      ; 21 06 00       !..  
00c1: add hl,de                      ; 19             .    
00c2: inc hl                         ; 23             #    
00c3: inc hl                         ; 23             #    
00c4: ld c,(hl)                      ; 4e             n    
00c5: inc hl                         ; 23             #    
00c6: ld b,(hl)                      ; 46             f    
00c7: push bc                        ; c5             .    
00c8: ld hl,0x4                      ; 21 04 00       !..  
00cb: add hl,de                      ; 19             .    
00cc: ld c,(hl)                      ; 4e             n    
00cd: inc hl                         ; 23             #    
00ce: ld b,(hl)                      ; 46             f    
00cf: push bc                        ; c5             .    
00d0: call 0x0                       ; cd 00 00       ...  
00d3: pop af                         ; f1             .    
00d4: pop af                         ; f1             .    
00d5: pop af                         ; f1             .    
00d6: ld a,b                         ; 78             x    
00d7: or a,a                         ; b7             .    
00d8: jp p,0xe7                      ; f2 e7 00       ...  
00db: ld bc,0xffff                   ; 01 ff ff       ...  
00de: jp 0x0                         ; c3 00 00       ...  
00e1: ld bc,0xffff                   ; 01 ff ff       ...  
00e4: jp 0x0                         ; c3 00 00       ...  
00e7: ld hl,0x4                      ; 21 04 00       !..  
00ea: add hl,de                      ; 19             .    
00eb: ld a,(hl)                      ; 7e             ~    
00ec: inc hl                         ; 23             #    
00ed: ld h,(hl)                      ; 66             f    
00ee: ld l,a                         ; 6f             o    
00ef: ld c,l                         ; 4d             m    
00f0: ld b,h                         ; 44             d    
00f1: jp 0x0                         ; c3 00 00       ...  
data:
00f4: 00 00 00 00                                      .... 
