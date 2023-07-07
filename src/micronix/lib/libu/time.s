;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

time.o:
    0    _errno: 0000 08 global 
    1     _time: 0000 0d global defined code 
0000: push de                        ; d5             .    
0001: sys time                       ; cf 0d          ..   
0003: ex de,hl                       ; eb             .    
0004: push hl                        ; e5             .    
0005: ld hl,0x6                      ; 21 06 00       !..  
0008: add hl,sp                      ; 39             9    
0009: ld a,(hl)                      ; 7e             ~    
000a: inc hl                         ; 23             #    
000b: ld h,(hl)                      ; 66             f    
000c: ld l,a                         ; 6f             o    
000d: ld (hl),e                      ; 73             s    
000e: inc hl                         ; 23             #    
000f: ld (hl),d                      ; 72             r    
0010: inc hl                         ; 23             #    
0011: pop de                         ; d1             .    
0012: ld (hl),e                      ; 73             s    
0013: inc hl                         ; 23             #    
0014: ld (hl),d                      ; 72             r    
0015: pop de                         ; d1             .    
0016: ld bc,0x0                      ; 01 00 00       ...  
0019: ret nc                         ; d0             .    
001a: dec bc                         ; 0b             .    
001b: ld (0x0),hl                    ; 22 00 00       "..  
001e: ret                            ; c9             .    
