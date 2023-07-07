;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

_exit.o:
    0    _errno: 0000 08 global 
    1    __exit: 0000 0d global defined code 
0000: pop bc                         ; c1             .    
0001: pop hl                         ; e1             .    
0002: push hl                        ; e5             .    
0003: push bc                        ; c5             .    
0004: sys exit                       ; cf 01          ..   
