;
; names for the recovered netdaemon.bin, for src/tools/disas
;
;	disas -f netdaemon.ctl netdaemon.bin
;
start 0x0100
define main 0x02aa		; called from the startup at 0x100
define cent 0x454d
define cret 0x4554

; the literal pools in the text are marked in the .dis
