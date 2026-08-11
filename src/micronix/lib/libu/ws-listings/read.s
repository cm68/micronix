;
; assembly source for read system call
;
; /usr/src/lib/libu/read.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

read.o:
    0    _errno: 0000 08 global 
    1     _read: 0000 0d global defined code 
0000: ld hl,0x4                      ; 21 04 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: ld (0x2e),hl                   ; 22 2e 00       "..  
000b: ld hl,0x6                      ; 21 06 00       !..  
000e: add hl,sp                      ; 39             9    
000f: ld a,(hl)                      ; 7e             ~    
0010: inc hl                         ; 23             #    
0011: ld h,(hl)                      ; 66             f    
0012: ld l,a                         ; 6f             o    
0013: ld (0x30),hl                   ; 22 30 00       "0.  
0016: ld hl,0x2                      ; 21 02 00       !..  
0019: add hl,sp                      ; 39             9    
001a: ld a,(hl)                      ; 7e             ~    
001b: inc hl                         ; 23             #    
001c: ld h,(hl)                      ; 66             f    
001d: ld l,a                         ; 6f             o    
001e: sys indir 2c 00                ; cf 00 2c 00    ..,. 
0022: ld c,l                         ; 4d             m    
0023: ld b,h                         ; 44             d    
0024: ret nc                         ; d0             .    
0025: ld bc,0xffff                   ; 01 ff ff       ...  
0028: ld (0x0),hl                    ; 22 00 00       "..  
002b: ret                            ; c9             .    
data:
002c: cf 03 00 00  00 00                               .... ..
