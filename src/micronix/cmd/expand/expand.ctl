; names for /usr/bin/expand off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f expand.ctl expand.dist
;
; There is no source for expand.  This file names what the reconstruction in
; expand.c had to know; see README beside this file.
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

define c.ent 0x1149
define c.ret 0x1150
define c.ents 0x0ffc
define c.rets 0x1012

; newuser's own code is the first few functions; everything after is library
main code 0x0132
usage code 0x034d	; "usage: expand [ input [ output ] ]."
cantopen code 0x0384	; name, ": can't open."
finit code 0x03b8	; attach a stream to a file descriptor
getc code 0x052c
putc code 0x05c6
create code 0x0d57	; creat(name, 0777)
open code 0x0f80
