;
; names for the recovered netmail16.bin, for src/tools/disas
;
;	disas -f netmail.ctl netmail16.bin
;
start 0x0100
define main 0x010e		; called from the startup at 0x100
define cent 0x3c06
define cret 0x3c0d

; the literal pools in the text are marked in the .dis
