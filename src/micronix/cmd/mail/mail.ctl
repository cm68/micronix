;
; names for the recovered mail16.bin, for src/tools/disas
;
;	disas -f mail.ctl mail16.bin
;
start 0x0100
define main 0x0159		; called from the startup at 0x100
define cent 0x44c8
define cret 0x44cf

; the literal pools in the text are marked in the .dis
