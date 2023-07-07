;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

creat.o:
    0    _errno: 0000 08 global 
    1    _creat: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: ld (0x26),hl                   ; 22 26 00       "&.  
000b: ld hl,0x4                      ; 21 04 00       !..  
000e: add hl,sp                      ; 39             9    
000f: ld a,(hl)                      ; 7e             ~    
0010: inc hl                         ; 23             #    
0011: ld h,(hl)                      ; 66             f    
0012: ld l,a                         ; 6f             o    
0013: ld (0x28),hl                   ; 22 28 00       "(.  
0016: sys indir 24 00                ; cf 00 24 00    ..$. 
001a: ld c,l                         ; 4d             m    
001b: ld b,h                         ; 44             d    
001c: ret nc                         ; d0             .    
001d: ld bc,0xffff                   ; 01 ff ff       ...  
0020: ld (0x0),hl                    ; 22 00 00       "..  
0023: ret                            ; c9             .    
data:
0024: cf 08 00 00  00 00                               .... ..
