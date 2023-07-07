;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

fork.o:
    0    _errno: 0000 08 global 
    1     _fork: 0000 0d global defined code 
0000: sys fork                       ; cf 02          ..   
0002: jp 0xf                         ; c3 0f 00       ...  
0005: ld c,l                         ; 4d             m    
0006: ld b,h                         ; 44             d    
0007: ret nc                         ; d0             .    
0008: ld bc,0xffff                   ; 01 ff ff       ...  
000b: ld (0x0),hl                    ; 22 00 00       "..  
000e: ret                            ; c9             .    
000f: ld bc,0x0                      ; 01 00 00       ...  
0012: ret                            ; c9             .    
