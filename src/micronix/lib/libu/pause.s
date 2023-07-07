;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

pause.o:
    0    _errno: 0000 08 global 
    1    _pause: 0000 0d global defined code 
0000: sys indir 08 00                ; cf 00 08 00    .... 
0004: ld bc,0x0                      ; 01 00 00       ...  
0007: ret                            ; c9             .    
data:
0008: cf 1d                                           ..
