;
; assembly source for close system call
;
; /usr/src/lib/libu/close.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

close.o:
    0    _errno: 0000 08 global 
    1    _close: 0000 0d global defined code 
0000: pop bc                         ; c1             .    
0001: pop hl                         ; e1             .    
0002: push hl                        ; e5             .    
0003: push bc                        ; c5             .    
0004: sys close                      ; cf 06          ..   
0006: ld bc,0x0                      ; 01 00 00       ...  
0009: ret nc                         ; d0             .    
000a: dec bc                         ; 0b             .    
000b: ld (0x0),hl                    ; 22 00 00       "..  
000e: ret                            ; c9             .    
