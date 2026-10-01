; names for /usr/bin/compress off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f compress.ctl compress.dist
;
; There is no source for compress.  This file names what the reconstruction in
; compress.c had to know; see README beside this file.
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

define c.ent 0x1341
define c.ret 0x1348
define c.ents 0x11f4
define c.rets 0x120a

; newuser's own code is the first few functions; everything after is library
main code 0x0134
putlit code 0x034a	; send the literal bytes held
usage code 0x038a	; "usage: compress [ input [ output ] ]."
cantopen code 0x03c1	; name, ": can't open."
finit code 0x03f5	; attach a stream to a file descriptor
getc code 0x0569
putc code 0x0603
write code 0x06d9	; a block, through the stream
remark code 0x0eae	; strings to a descriptor
exit code 0x0df3
create code 0x0f4f	; creat(name, 0777)
