;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;

remove.o:
    0   _unlink: 0000 08 global 
    1     c.ret: 0000 08 global 
    2     c.ent: 0000 08 global 
    3   _remove: 0000 0d global defined code 
0000: call 0x0                       ; cd 00 00       ...  
0003: ld hl,0x4                      ; 21 04 00       !..  
0006: add hl,de                      ; 19             .    
0007: ld c,(hl)                      ; 4e             n    
0008: inc hl                         ; 23             #    
0009: ld b,(hl)                      ; 46             f    
000a: push bc                        ; c5             .    
000b: call 0x0                       ; cd 00 00       ...  
000e: pop af                         ; f1             .    
000f: jp 0x0                         ; c3 00 00       ...  
