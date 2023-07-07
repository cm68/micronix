;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

nice.o:
    0    _errno: 0000 08 global 
    1     _nice: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: sys nice                       ; cf 22          ."   
000a: ld bc,0x0                      ; 01 00 00       ...  
000d: ret nc                         ; d0             .    
000e: dec bc                         ; 0b             .    
000f: ld (0x0),hl                    ; 22 00 00       "..  
0012: ret                            ; c9             .    
