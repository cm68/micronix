;
; assembly source for execv system call
;
; /usr/src/lib/libu/execv.s
;
; Changed: <2023-07-07 01:10:10 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

execv.o:
    0     c.ret: 0000 08 global 
    1     c.ent: 0000 08 global 
    2     _exec: 0000 08 global 
    3    _execv: 0000 0d global defined code 
    4    _execl: 001b 0d global defined code 
0000: call 0x0                       ; cd 00 00       ...  
0003: ld hl,0x6                      ; 21 06 00       !..  
0006: add hl,de                      ; 19             .    
0007: ld c,(hl)                      ; 4e             n    
0008: inc hl                         ; 23             #    
0009: ld b,(hl)                      ; 46             f    
000a: push bc                        ; c5             .    
000b: ld hl,0x4                      ; 21 04 00       !..  
000e: add hl,de                      ; 19             .    
000f: ld c,(hl)                      ; 4e             n    
0010: inc hl                         ; 23             #    
0011: ld b,(hl)                      ; 46             f    
0012: push bc                        ; c5             .    
0013: call 0x0                       ; cd 00 00       ...  
0016: pop af                         ; f1             .    
0017: pop af                         ; f1             .    
0018: jp 0x0                         ; c3 00 00       ...  
001b: call 0x0                       ; cd 00 00       ...  
001e: ld hl,0x6                      ; 21 06 00       !..  
0021: add hl,de                      ; 19             .    
0022: push hl                        ; e5             .    
0023: ld hl,0x4                      ; 21 04 00       !..  
0026: add hl,de                      ; 19             .    
0027: ld c,(hl)                      ; 4e             n    
0028: inc hl                         ; 23             #    
0029: ld b,(hl)                      ; 46             f    
002a: push bc                        ; c5             .    
002b: call 0x0                       ; cd 00 00       ...  
002e: pop af                         ; f1             .    
002f: pop af                         ; f1             .    
0030: jp 0x0                         ; c3 00 00       ...  
