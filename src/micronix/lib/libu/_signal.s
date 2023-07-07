;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

_signal.o:
    0     c.ihl: 0000 08 global 
    1    _errno: 0000 08 global 
    2  __signal: 0000 0d global defined code 
    3    __stab: 009e 0e global defined data 
    4    __jtab: 0024 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: ld (0x9a),hl                   ; 22 9a 00       "..  
000b: ld hl,0x4                      ; 21 04 00       !..  
000e: add hl,sp                      ; 39             9    
000f: ld a,(hl)                      ; 7e             ~    
0010: inc hl                         ; 23             #    
0011: ld h,(hl)                      ; 66             f    
0012: ld l,a                         ; 6f             o    
0013: ld (0x9c),hl                   ; 22 9c 00       "..  
0016: sys indir 98 00                ; cf 00 98 00    .... 
001a: ld c,l                         ; 4d             m    
001b: ld b,h                         ; 44             d    
001c: ret nc                         ; d0             .    
001d: ld bc,0xffff                   ; 01 ff ff       ...  
0020: ld (0x0),hl                    ; 22 00 00       "..  
0023: ret                            ; c9             .    
0024: push hl                        ; e5             .    
0025: ld hl,(0x9e)                   ; 2a 9e 00       *..  
0028: jp 0x8d                        ; c3 8d 00       ...  
002b: push hl                        ; e5             .    
002c: ld hl,(0xa0)                   ; 2a a0 00       *..  
002f: jp 0x8d                        ; c3 8d 00       ...  
0032: push hl                        ; e5             .    
0033: ld hl,(0xa2)                   ; 2a a2 00       *..  
0036: jp 0x8d                        ; c3 8d 00       ...  
0039: push hl                        ; e5             .    
003a: ld hl,(0xa4)                   ; 2a a4 00       *..  
003d: jp 0x8d                        ; c3 8d 00       ...  
0040: push hl                        ; e5             .    
0041: ld hl,(0xa6)                   ; 2a a6 00       *..  
0044: jp 0x8d                        ; c3 8d 00       ...  
0047: push hl                        ; e5             .    
0048: ld hl,(0xa8)                   ; 2a a8 00       *..  
004b: jp 0x8d                        ; c3 8d 00       ...  
004e: push hl                        ; e5             .    
004f: ld hl,(0xaa)                   ; 2a aa 00       *..  
0052: jp 0x8d                        ; c3 8d 00       ...  
0055: push hl                        ; e5             .    
0056: ld hl,(0xac)                   ; 2a ac 00       *..  
0059: jp 0x8d                        ; c3 8d 00       ...  
005c: push hl                        ; e5             .    
005d: ld hl,(0xae)                   ; 2a ae 00       *..  
0060: jp 0x8d                        ; c3 8d 00       ...  
0063: push hl                        ; e5             .    
0064: ld hl,(0xb0)                   ; 2a b0 00       *..  
0067: jp 0x8d                        ; c3 8d 00       ...  
006a: push hl                        ; e5             .    
006b: ld hl,(0xb2)                   ; 2a b2 00       *..  
006e: jp 0x8d                        ; c3 8d 00       ...  
0071: push hl                        ; e5             .    
0072: ld hl,(0xb4)                   ; 2a b4 00       *..  
0075: jp 0x8d                        ; c3 8d 00       ...  
0078: push hl                        ; e5             .    
0079: ld hl,(0xb6)                   ; 2a b6 00       *..  
007c: jp 0x8d                        ; c3 8d 00       ...  
007f: push hl                        ; e5             .    
0080: ld hl,(0xb8)                   ; 2a b8 00       *..  
0083: jp 0x8d                        ; c3 8d 00       ...  
0086: push hl                        ; e5             .    
0087: ld hl,(0xba)                   ; 2a ba 00       *..  
008a: jp 0x8d                        ; c3 8d 00       ...  
008d: push de                        ; d5             .    
008e: push bc                        ; c5             .    
008f: push af                        ; f5             .    
0090: call 0x0                       ; cd 00 00       ...  
0093: pop af                         ; f1             .    
0094: pop bc                         ; c1             .    
0095: pop de                         ; d1             .    
0096: pop hl                         ; e1             .    
0097: ret                            ; c9             .    
data:
0098: cf 30 00 00  00 00 00 00  00 00 00 00  00 00 00 00  .0.. .... .... .... 
00a8: 00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  .... .... .... .... 
00b8: 00 00 00 00                                      .... 
