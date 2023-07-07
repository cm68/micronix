;
; assembly source for getuid system call
;
; /usr/src/lib/libu/getuid.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

getuid.o:
    0    _errno: 0000 08 global 
    1   _getuid: 0000 0d global defined code 
0000: sys getuid                     ; cf 18          ..   
0002: ld c,l                         ; 4d             m    
0003: ld b,h                         ; 44             d    
0004: ret                            ; c9             .    
