;
; assembly source for sleep system call
;
; /usr/src/lib/libu/sleep.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

sleep.o:
    0    _errno: 0000 08 global 
    1    _sleep: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: sys sleep                      ; cf 23          .#   
000a: ld bc,0x0                      ; 01 00 00       ...  
000d: ret                            ; c9             .    
