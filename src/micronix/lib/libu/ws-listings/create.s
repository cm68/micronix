;
; assembly source for create system call
;
; /usr/src/lib/libu/create.s
;
; Changed: <2023-07-07 01:04:57 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

create.o:
    0    _close: 0000 08 global 
    1    _creat: 0000 08 global 
    2     c.ret: 0000 08 global 
    3     c.ent: 0000 08 global 
    4     _open: 0000 08 global 
    5   _create: 0000 0d global defined code 
0000: call 0x0                       ; cd 00 00       ...  
0003: push af                        ; f5             .    
0004: push af                        ; f5             .    
0005: push af                        ; f5             .    
0006: push af                        ; f5             .    
0007: ld hl,0xfff8                   ; 21 f8 ff       !..  
000a: add hl,de                      ; 19             .    
000b: push hl                        ; e5             .    
000c: ld hl,0x1ff                    ; 21 ff 01       !..  
000f: push hl                        ; e5             .    
0010: ld hl,0x4                      ; 21 04 00       !..  
0013: add hl,de                      ; 19             .    
0014: ld c,(hl)                      ; 4e             n    
0015: inc hl                         ; 23             #    
0016: ld b,(hl)                      ; 46             f    
0017: push bc                        ; c5             .    
0018: call 0x0                       ; cd 00 00       ...  
001b: pop af                         ; f1             .    
001c: pop af                         ; f1             .    
001d: pop hl                         ; e1             .    
001e: ld a,c                         ; 79             y    
001f: ld (hl),a                      ; 77             w    
0020: ld a,b                         ; 78             x    
0021: inc hl                         ; 23             #    
0022: ld (hl),a                      ; 77             w    
0023: ld hl,0xfff8                   ; 21 f8 ff       !..  
0026: add hl,de                      ; 19             .    
0027: inc hl                         ; 23             #    
0028: ld a,(hl)                      ; 7e             ~    
0029: or a,a                         ; b7             .    
002a: jp p,0x3a                      ; f2 3a 00       .:.  
002d: ld hl,0xfff8                   ; 21 f8 ff       !..  
0030: add hl,de                      ; 19             .    
0031: ld a,(hl)                      ; 7e             ~    
0032: inc hl                         ; 23             #    
0033: ld h,(hl)                      ; 66             f    
0034: ld l,a                         ; 6f             o    
0035: ld c,l                         ; 4d             m    
0036: ld b,h                         ; 44             d    
0037: jp 0x0                         ; c3 00 00       ...  
003a: ld hl,0x6                      ; 21 06 00       !..  
003d: add hl,de                      ; 19             .    
003e: ld a,(hl)                      ; 7e             ~    
003f: inc hl                         ; 23             #    
0040: or a,(hl)                      ; b6             .    
0041: jp nz,0x6f                     ; c2 6f 00       .o.  
0044: ld hl,0xfff8                   ; 21 f8 ff       !..  
0047: add hl,de                      ; 19             .    
0048: ld c,(hl)                      ; 4e             n    
0049: inc hl                         ; 23             #    
004a: ld b,(hl)                      ; 46             f    
004b: push bc                        ; c5             .    
004c: call 0x0                       ; cd 00 00       ...  
004f: pop af                         ; f1             .    
0050: ld hl,0xfff8                   ; 21 f8 ff       !..  
0053: add hl,de                      ; 19             .    
0054: push hl                        ; e5             .    
0055: ld hl,0x0                      ; 21 00 00       !..  
0058: push hl                        ; e5             .    
0059: ld hl,0x4                      ; 21 04 00       !..  
005c: add hl,de                      ; 19             .    
005d: ld c,(hl)                      ; 4e             n    
005e: inc hl                         ; 23             #    
005f: ld b,(hl)                      ; 46             f    
0060: push bc                        ; c5             .    
0061: call 0x0                       ; cd 00 00       ...  
0064: pop af                         ; f1             .    
0065: pop af                         ; f1             .    
0066: pop hl                         ; e1             .    
0067: ld a,c                         ; 79             y    
0068: ld (hl),a                      ; 77             w    
0069: ld a,b                         ; 78             x    
006a: inc hl                         ; 23             #    
006b: ld (hl),a                      ; 77             w    
006c: jp 0xa8                        ; c3 a8 00       ...  
006f: ld hl,0x6                      ; 21 06 00       !..  
0072: add hl,de                      ; 19             .    
0073: ld a,(hl)                      ; 7e             ~    
0074: cp a,0x2                       ; fe 02          ..   
0076: jp nz,0x7d                     ; c2 7d 00       .}.  
0079: inc hl                         ; 23             #    
007a: ld a,(hl)                      ; 7e             ~    
007b: cp a,0x0                       ; fe 00          ..   
007d: jp nz,0xa8                     ; c2 a8 00       ...  
0080: ld hl,0xfff8                   ; 21 f8 ff       !..  
0083: add hl,de                      ; 19             .    
0084: ld c,(hl)                      ; 4e             n    
0085: inc hl                         ; 23             #    
0086: ld b,(hl)                      ; 46             f    
0087: push bc                        ; c5             .    
0088: call 0x0                       ; cd 00 00       ...  
008b: pop af                         ; f1             .    
008c: ld hl,0xfff8                   ; 21 f8 ff       !..  
008f: add hl,de                      ; 19             .    
0090: push hl                        ; e5             .    
0091: ld hl,0x2                      ; 21 02 00       !..  
0094: push hl                        ; e5             .    
0095: ld hl,0x4                      ; 21 04 00       !..  
0098: add hl,de                      ; 19             .    
0099: ld c,(hl)                      ; 4e             n    
009a: inc hl                         ; 23             #    
009b: ld b,(hl)                      ; 46             f    
009c: push bc                        ; c5             .    
009d: call 0x0                       ; cd 00 00       ...  
00a0: pop af                         ; f1             .    
00a1: pop af                         ; f1             .    
00a2: pop hl                         ; e1             .    
00a3: ld a,c                         ; 79             y    
00a4: ld (hl),a                      ; 77             w    
00a5: ld a,b                         ; 78             x    
00a6: inc hl                         ; 23             #    
00a7: ld (hl),a                      ; 77             w    
00a8: ld hl,0xfff8                   ; 21 f8 ff       !..  
00ab: add hl,de                      ; 19             .    
00ac: ld a,(hl)                      ; 7e             ~    
00ad: inc hl                         ; 23             #    
00ae: ld h,(hl)                      ; 66             f    
00af: ld l,a                         ; 6f             o    
00b0: ld c,l                         ; 4d             m    
00b1: ld b,h                         ; 44             d    
00b2: jp 0x0                         ; c3 00 00       ...  
