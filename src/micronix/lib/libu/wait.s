;
; assembly source for wait system call
;
; /usr/src/lib/libu/wait.s
;
; Changed: <2023-07-07 01:46:32 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

wait.o:
    0    _errno: 0000 08 global 
    1     _wait: 0000 0d global defined code 
0000: push de                        ; d5             .    
0001: sys wait                       ; cf 07          ..   
0003: jp c,0x15                      ; da 15 00       ...  
0006: ld c,l                         ; 4d             m    
0007: ld b,h                         ; 44             d    
0008: ld hl,0x4                      ; 21 04 00       !..  
000b: add hl,sp                      ; 39             9    
000c: ld a,(hl)                      ; 7e             ~    
000d: inc hl                         ; 23             #    
000e: ld h,(hl)                      ; 66             f    
000f: ld l,a                         ; 6f             o    
0010: ld (hl),e                      ; 73             s    
0011: inc hl                         ; 23             #    
0012: ld (hl),d                      ; 72             r    
0013: pop de                         ; d1             .    
0014: ret                            ; c9             .    
0015: pop de                         ; d1             .    
0016: ld bc,0xffff                   ; 01 ff ff       ...  
0019: ld (0x0),hl                    ; 22 00 00       "..  
001c: ret                            ; c9             .    
