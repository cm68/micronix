;
; names for the recovered netcp.bin, for src/tools/disas
;
;	disas -f netcp.ctl netcp.bin
;
start 0x0100
define main 0x010e		; called from the startup at 0x100
define cent 0x44f9
define cret 0x4500

; the literal pools in the text are marked in the .dis
