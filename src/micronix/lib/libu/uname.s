;
; assembly source for uname system call
;
; /usr/src/lib/libu/uname.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

uname.o:
    0      c.r0: 0000 08 global 
    1   c.ulmod: 0000 08 global 
    2     c.ret: 0000 08 global 
    3   _getpid: 0000 08 global 
    4     c.ent: 0000 08 global 
    5     _ltob: 0000 08 global 
    6     _time: 0000 08 global 
    7     _itob: 0000 08 global 
    8   _cpystr: 0000 08 global 
    9    _uname: 0006 0d global defined code 
0000: cpl                            ; 2f             /    
0001: ld (hl),h                      ; 74             t    
0002: ld l,l                         ; 6d             m    
0003: ld (hl),b                      ; 70             p    
0004: cpl                            ; 2f             /    
0005: nop                            ; 00             .    
0006: call 0x0                       ; cd 00 00       ...  
0009: ld hl,0xc1                     ; 21 c1 00       !..  
000c: ld a,(hl)                      ; 7e             ~    
000d: inc hl                         ; 23             #    
000e: or a,(hl)                      ; b6             .    
000f: inc hl                         ; 23             #    
0010: or a,(hl)                      ; b6             .    
0011: inc hl                         ; 23             #    
0012: or a,(hl)                      ; b6             .    
0013: jp nz,0x3e                     ; c2 3e 00       .>.  
0016: ld hl,0xc1                     ; 21 c1 00       !..  
0019: push hl                        ; e5             .    
001a: call 0x0                       ; cd 00 00       ...  
001d: pop af                         ; f1             .    
001e: ld hl,0xc1                     ; 21 c1 00       !..  
0021: push hl                        ; e5             .    
0022: ld a,0x27                      ; 3e 27          >'   
0024: add a,a                        ; 87             .    
0025: sbc a,a                        ; 9f             .    
0026: ld (0x0),a                     ; 32 00 00       2..  
0029: ld (0x1),a                     ; 32 01 00       2..  
002c: ld a,0x27                      ; 3e 27          >'   
002e: ld (0x3),a                     ; 32 03 00       2..  
0031: ld a,0x10                      ; 3e 10          >.   
0033: ld (0x2),a                     ; 32 02 00       2..  
0036: ld hl,0x0                      ; 21 00 00       !..  
0039: push hl                        ; e5             .    
003a: call 0x0                       ; cd 00 00       ...  
003d: pop af                         ; f1             .    
003e: ld hl,0x0                      ; 21 00 00       !..  
0041: push hl                        ; e5             .    
0042: ld hl,0x0                      ; 21 00 00       !..  
0045: push hl                        ; e5             .    
0046: ld hl,0xaf                     ; 21 af 00       !..  
0049: push hl                        ; e5             .    
004a: call 0x0                       ; cd 00 00       ...  
004d: pop af                         ; f1             .    
004e: pop af                         ; f1             .    
004f: pop af                         ; f1             .    
0050: ld l,c                         ; 69             i    
0051: ld h,b                         ; 60             `    
0052: ld (0xbf),hl                   ; 22 bf 00       "..  
0055: ld hl,0xa                      ; 21 0a 00       !..  
0058: push hl                        ; e5             .    
0059: call 0x0                       ; cd 00 00       ...  
005c: push bc                        ; c5             .    
005d: ld hl,(0xbf)                   ; 2a bf 00       *..  
0060: push hl                        ; e5             .    
0061: call 0x0                       ; cd 00 00       ...  
0064: pop af                         ; f1             .    
0065: pop af                         ; f1             .    
0066: pop af                         ; f1             .    
0067: ld hl,(0xbf)                   ; 2a bf 00       *..  
006a: add hl,bc                      ; 09             .    
006b: ld (0xbf),hl                   ; 22 bf 00       "..  
006e: ld hl,(0xbf)                   ; 2a bf 00       *..  
0071: push hl                        ; e5             .    
0072: ld hl,(0xbf)                   ; 2a bf 00       *..  
0075: inc hl                         ; 23             #    
0076: ld (0xbf),hl                   ; 22 bf 00       "..  
0079: pop hl                         ; e1             .    
007a: ld (hl),0x2d                   ; 36 2d          6-   
007c: ld hl,0xa                      ; 21 0a 00       !..  
007f: push hl                        ; e5             .    
0080: ld hl,0xc1                     ; 21 c1 00       !..  
0083: inc hl                         ; 23             #    
0084: inc hl                         ; 23             #    
0085: ld c,(hl)                      ; 4e             n    
0086: inc hl                         ; 23             #    
0087: ld b,(hl)                      ; 46             f    
0088: push bc                        ; c5             .    
0089: dec hl                         ; 2b             +    
008a: dec hl                         ; 2b             +    
008b: dec hl                         ; 2b             +    
008c: ld c,(hl)                      ; 4e             n    
008d: inc hl                         ; 23             #    
008e: ld b,(hl)                      ; 46             f    
008f: push bc                        ; c5             .    
0090: ld hl,(0xbf)                   ; 2a bf 00       *..  
0093: push hl                        ; e5             .    
0094: call 0x0                       ; cd 00 00       ...  
0097: pop af                         ; f1             .    
0098: pop af                         ; f1             .    
0099: pop af                         ; f1             .    
009a: pop af                         ; f1             .    
009b: ld hl,(0xbf)                   ; 2a bf 00       *..  
009e: add hl,bc                      ; 09             .    
009f: ld (0xbf),hl                   ; 22 bf 00       "..  
00a2: ld hl,(0xbf)                   ; 2a bf 00       *..  
00a5: ld (hl),0x0                    ; 36 00          6.   
00a7: ld hl,0xaf                     ; 21 af 00       !..  
00aa: ld c,l                         ; 4d             m    
00ab: ld b,h                         ; 44             d    
00ac: jp 0x0                         ; c3 00 00       ...  
data:
00af: 00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  .... .... .... .... 
00bf: 00 00 00 00  00 00                               .... ..
