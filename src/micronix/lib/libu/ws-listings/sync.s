;
; assembly source for sync system call
;
; /usr/src/lib/libu/sync.s
;
; Changed: <2023-07-07 01:44:23 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

sync.o:
    0    _errno: 0000 08 global 
    1     _sync: 0000 0d global defined code 
0000: sys sync                       ; cf 24          .$   
0002: ld bc,0x0                      ; 01 00 00       ...  
0005: ret                            ; c9             .    
