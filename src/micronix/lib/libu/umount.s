;
; assembly source for umount system call
;
; /usr/src/lib/libu/umount.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

umount.o:
    0    _errno: 0000 08 global 
    1   _umount: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: ld (0x1a),hl                   ; 22 1a 00       "..  
000b: sys indir 18 00                ; cf 00 18 00    .... 
000f: ld bc,0x0                      ; 01 00 00       ...  
0012: ret nc                         ; d0             .    
0013: dec bc                         ; 0b             .    
0014: ld (0x0),hl                    ; 22 00 00       "..  
0017: ret                            ; c9             .    
data:
0018: cf 16 00 00                                      .... 
