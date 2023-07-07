;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

stime.o:
    0    _errno: 0000 08 global 
    1    _stime: 0000 0d global defined code 
0000: push de                        ; d5             .    
0001: ld hl,0x4                      ; 21 04 00       !..  
0004: add hl,sp                      ; 39             9    
0005: ld a,(hl)                      ; 7e             ~    
0006: inc hl                         ; 23             #    
0007: ld h,(hl)                      ; 66             f    
0008: ld l,a                         ; 6f             o    
0009: ld e,(hl)                      ; 5e             ^    
000a: inc hl                         ; 23             #    
000b: ld d,(hl)                      ; 56             v    
000c: inc hl                         ; 23             #    
000d: ld a,(hl)                      ; 7e             ~    
000e: inc hl                         ; 23             #    
000f: ld h,(hl)                      ; 66             f    
0010: ld l,a                         ; 6f             o    
0011: ex de,hl                       ; eb             .    
0012: sys stime                      ; cf 19          ..   
0014: pop de                         ; d1             .    
0015: ld bc,0x0                      ; 01 00 00       ...  
0018: ret nc                         ; d0             .    
0019: dec bc                         ; 0b             .    
001a: ld (0x0),hl                    ; 22 00 00       "..  
001d: ret                            ; c9             .    
