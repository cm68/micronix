;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

seek.o:
    0    _errno: 0000 08 global 
    1     _seek: 0000 0d global defined code 
0000: ld hl,0x4                      ; 21 04 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: ld (0x2d),hl                   ; 22 2d 00       "-.  
000b: ld hl,0x6                      ; 21 06 00       !..  
000e: add hl,sp                      ; 39             9    
000f: ld a,(hl)                      ; 7e             ~    
0010: inc hl                         ; 23             #    
0011: ld h,(hl)                      ; 66             f    
0012: ld l,a                         ; 6f             o    
0013: ld (0x2f),hl                   ; 22 2f 00       "/.  
0016: ld hl,0x2                      ; 21 02 00       !..  
0019: add hl,sp                      ; 39             9    
001a: ld a,(hl)                      ; 7e             ~    
001b: inc hl                         ; 23             #    
001c: ld h,(hl)                      ; 66             f    
001d: ld l,a                         ; 6f             o    
001e: sys indir 2b 00                ; cf 00 2b 00    ..+. 
0022: ld bc,0x0                      ; 01 00 00       ...  
0025: ret nc                         ; d0             .    
0026: dec bc                         ; 0b             .    
0027: ld (0x0),hl                    ; 22 00 00       "..  
002a: ret                            ; c9             .    
data:
002b: cf 13 00 00  00 00                               .... ..
