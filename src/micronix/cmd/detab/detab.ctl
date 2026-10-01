; names for /usr/bin/detab off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f detab.ctl detab.dist
;
; There is no source for detab.  This file names what the reconstruction in
; detab.c had to know; see README beside this file.
;
; The binary has no header: it is the "old cpm format" the kernel's exec
; accepts, the whole file loaded as text at 0100 and entered there.  It is
; Whitesmith's C, the same runtime newuser, login and init use: c.ent and
; c.ret are the plain prologue and epilogue, c.ents and c.rets the pair
; that saves the three register variable cells.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x.... .
;
start 0x0100

define c.ent 0x0d9f
define c.ret 0x0da6
define c.ents 0x0c52
define c.rets 0x0c68

; newuser's own code is the first few functions; everything after is library
main code 0x010e	; a loop over getchar: tab, newline, anything else
attabstop code 0x0192	; column mod 8 equals 1
getchar code 0x01cb
getc code 0x01d9
putchar code 0x0245
putc code 0x0273
exit code 0x09a9
