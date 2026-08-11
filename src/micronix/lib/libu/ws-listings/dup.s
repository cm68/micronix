;
; assembly source for dup system call
;
; /usr/src/lib/libu/dup.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

dup.o:
    0    _errno: 0000 08 global 
    1      _dup: 0000 0d global defined code 
0000: ld hl,0x2                      ; 21 02 00       !..  
0003: add hl,sp                      ; 39             9    
0004: ld a,(hl)                      ; 7e             ~    
0005: inc hl                         ; 23             #    
0006: ld h,(hl)                      ; 66             f    
0007: ld l,a                         ; 6f             o    
0008: sys dup                        ; cf 29          .)   
000a: ld c,l                         ; 4d             m    
000b: ld b,h                         ; 44             d    
000c: ret nc                         ; d0             .    
000d: ld bc,0xffff                   ; 01 ff ff       ...  
0010: ld (0x0),hl                    ; 22 00 00       "..  
0013: ret                            ; c9             .    
