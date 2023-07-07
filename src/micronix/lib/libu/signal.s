;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

signal.o:
    0  __signal: 0000 08 global 
    1      c.r4: 0000 08 global 
    2      c.r2: 0000 08 global 
    3    __stab: 0000 08 global 
    4    __jtab: 0000 08 global 
    5    c.rets: 0000 08 global 
    6    c.ents: 0000 08 global 
    7   _signal: 0000 0d global defined code 
0000: call 0x0                       ; cd 00 00       ...  
0003: ld hl,0x4                      ; 21 04 00       !..  
0006: add hl,de                      ; 19             .    
0007: ld a,(hl)                      ; 7e             ~    
0008: inc hl                         ; 23             #    
0009: ld h,(hl)                      ; 66             f    
000a: ld l,a                         ; 6f             o    
000b: ld (0x0),hl                    ; 22 00 00       "..  
000e: ld hl,0x6                      ; 21 06 00       !..  
0011: add hl,de                      ; 19             .    
0012: ld a,(hl)                      ; 7e             ~    
0013: inc hl                         ; 23             #    
0014: ld h,(hl)                      ; 66             f    
0015: ld l,a                         ; 6f             o    
0016: ld (0x0),hl                    ; 22 00 00       "..  
0019: ld a,(0x0)                     ; 3a 00 00       :..  
001c: sub a,0x1                      ; d6 01          ..   
001e: ld a,(0x1)                     ; 3a 01 00       :..  
0021: sbc a,0x0                      ; de 00          ..   
0023: jp m,0x33                      ; fa 33 00       .3.  
0026: ld hl,0x0                      ; 21 00 00       !..  
0029: ld a,0xf                       ; 3e 0f          >.   
002b: sub a,(hl)                     ; 96             .    
002c: ld a,0x0                       ; 3e 00          >.   
002e: inc hl                         ; 23             #    
002f: sbc a,(hl)                     ; 9e             .    
0030: jp p,0x39                      ; f2 39 00       .9.  
0033: ld bc,0xffff                   ; 01 ff ff       ...  
0036: jp 0x0                         ; c3 00 00       ...  
0039: ld a,(0x0)                     ; 3a 00 00       :..  
003c: cp a,0x1                       ; fe 01          ..   
003e: jp nz,0x46                     ; c2 46 00       .f.  
0041: ld a,(0x1)                     ; 3a 01 00       :..  
0044: cp a,0x0                       ; fe 00          ..   
0046: jp z,0x58                      ; ca 58 00       .x.  
0049: ld hl,0x0                      ; 21 00 00       !..  
004c: ld a,(hl)                      ; 7e             ~    
004d: inc hl                         ; 23             #    
004e: or a,(hl)                      ; b6             .    
004f: jp z,0x58                      ; ca 58 00       .x.  
0052: ld bc,0x1                      ; 01 01 00       ...  
0055: jp 0x5d                        ; c3 5d 00       .].  
0058: ld hl,(0x0)                    ; 2a 00 00       *..  
005b: ld c,l                         ; 4d             m    
005c: ld b,h                         ; 44             d    
005d: push bc                        ; c5             .    
005e: ld hl,(0x0)                    ; 2a 00 00       *..  
0061: push hl                        ; e5             .    
0062: call 0x0                       ; cd 00 00       ...  
0065: pop af                         ; f1             .    
0066: pop af                         ; f1             .    
0067: ld l,c                         ; 69             i    
0068: ld h,b                         ; 60             `    
0069: ld (0xeb),hl                   ; 22 eb 00       "..  
006c: ld a,(0xeb)                    ; 3a eb 00       :..  
006f: cp a,0x1                       ; fe 01          ..   
0071: jp nz,0x79                     ; c2 79 00       .y.  
0074: ld a,(0xec)                    ; 3a ec 00       :..  
0077: cp a,0x0                       ; fe 00          ..   
0079: jp z,0x98                      ; ca 98 00       ...  
007c: ld hl,0xeb                     ; 21 eb 00       !..  
007f: ld a,(hl)                      ; 7e             ~    
0080: inc hl                         ; 23             #    
0081: or a,(hl)                      ; b6             .    
0082: jp z,0x98                      ; ca 98 00       ...  
0085: ld hl,(0x0)                    ; 2a 00 00       *..  
0088: ld bc,0xffff                   ; 01 ff ff       ...  
008b: add hl,bc                      ; 09             .    
008c: add hl,hl                      ; 29             )    
008d: ld bc,0x0                      ; 01 00 00       ...  
0090: add hl,bc                      ; 09             .    
0091: ld a,(hl)                      ; 7e             ~    
0092: inc hl                         ; 23             #    
0093: ld h,(hl)                      ; 66             f    
0094: ld l,a                         ; 6f             o    
0095: ld (0xeb),hl                   ; 22 eb 00       "..  
0098: ld a,(0x0)                     ; 3a 00 00       :..  
009b: cp a,0x1                       ; fe 01          ..   
009d: jp nz,0xa5                     ; c2 a5 00       ...  
00a0: ld a,(0x1)                     ; 3a 01 00       :..  
00a3: cp a,0x0                       ; fe 00          ..   
00a5: jp z,0xe3                      ; ca e3 00       ...  
00a8: ld hl,0x0                      ; 21 00 00       !..  
00ab: ld a,(hl)                      ; 7e             ~    
00ac: inc hl                         ; 23             #    
00ad: or a,(hl)                      ; b6             .    
00ae: jp z,0xe3                      ; ca e3 00       ...  
00b1: ld hl,(0x0)                    ; 2a 00 00       *..  
00b4: ld bc,0xffff                   ; 01 ff ff       ...  
00b7: add hl,bc                      ; 09             .    
00b8: add hl,hl                      ; 29             )    
00b9: ld bc,0x0                      ; 01 00 00       ...  
00bc: add hl,bc                      ; 09             .    
00bd: ld a,(0x0)                     ; 3a 00 00       :..  
00c0: ld (hl),a                      ; 77             w    
00c1: ld a,(0x1)                     ; 3a 01 00       :..  
00c4: inc hl                         ; 23             #    
00c5: ld (hl),a                      ; 77             w    
00c6: ld hl,(0x0)                    ; 2a 00 00       *..  
00c9: ld bc,0xffff                   ; 01 ff ff       ...  
00cc: add hl,bc                      ; 09             .    
00cd: push bc                        ; c5             .    
00ce: ld c,l                         ; 4d             m    
00cf: ld b,h                         ; 44             d    
00d0: add hl,hl                      ; 29             )    
00d1: add hl,bc                      ; 09             .    
00d2: add hl,hl                      ; 29             )    
00d3: add hl,bc                      ; 09             .    
00d4: pop bc                         ; c1             .    
00d5: ld bc,0x0                      ; 01 00 00       ...  
00d8: add hl,bc                      ; 09             .    
00d9: push hl                        ; e5             .    
00da: ld hl,(0x0)                    ; 2a 00 00       *..  
00dd: push hl                        ; e5             .    
00de: call 0x0                       ; cd 00 00       ...  
00e1: pop af                         ; f1             .    
00e2: pop af                         ; f1             .    
00e3: ld hl,(0xeb)                   ; 2a eb 00       *..  
00e6: ld c,l                         ; 4d             m    
00e7: ld b,h                         ; 44             d    
00e8: jp 0x0                         ; c3 00 00       ...  
data:
00eb: 00 00                                           ..
