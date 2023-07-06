;
; micronix crt0.s
;
; /src/lib/uhdr.o
;
; Changed: <2023-07-04 11:50:56 curt>
;
.extern _main, _exit

_crt0:	pop     bc
        ld      hl,0000H        ; 0001 !..        21 00 00 
        add     hl,sp           ; 0004 9          39 
        push    hl              ; 0005 .          e5 
        push    bc              ; 0006 .          c5 
        call    _main           ; 0007 ...        cd 00 00 
        push    bc              ; 000a .          c5 
        call    _exit           ; 000b ...        cd 00 00 
