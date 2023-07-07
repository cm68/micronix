;
; assembly source for pause system call
; not found in unix v6
; 
; /usr/src/lib/libu/pause.s
;
; Changed: <2023-07-07 01:33:25 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

pause.o:
    0    _errno: 0000 08 global 
    1    _pause: 0000 0d global defined code 
0000: sys indir 08 00                ; cf 00 08 00    .... 
0004: ld bc,0x0                      ; 01 00 00       ...  
0007: ret                            ; c9             .    
data:
0008: cf 1d                                           ..
