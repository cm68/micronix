;
; assembly source for stat system call
;
; /usr/src/lib/libu/stat.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

stat.o:
    0    _errno: 0000 08 global 
    1     _stat: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: ld (0x25),hl                   ; 22 25 00       "%.  
000b: ld hl,0x4                      ; 21 04 00       !..  
000e: add hl,sp                      ; 39             9    
000f: ld a,(hl)                      ; 7e             ~    
0010: inc hl                         ; 23             #    
0011: ld h,(hl)                      ; 66             f    
0012: ld l,a                         ; 6f             o    
0013: ld (0x27),hl                   ; 22 27 00       "'.  
0016: sys indir 23 00                ; cf 00 23 00    ..#. 
001a: ld bc,0x0                      ; 01 00 00       ...  
001d: ret nc                         ; d0             .    
001e: dec bc                         ; 0b             .    
001f: ld (0x0),hl                    ; 22 00 00       "..  
0022: ret                            ; c9             .    
data:
0023: cf 12 00 00  00 00                               .... ..
