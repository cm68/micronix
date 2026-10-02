; names for /usr/bin/concat off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f concat.ctl concat.dist
;
; There is no source for concat.  See README beside this file.  It has no
; header: the whole file is text at 0100, in Whitesmith's runtime, c.ent and
; c.ret the plain pair and c.ents and c.rets the pair that saves the register
; variable cells.  Every address here is written 0x...., which disas reads as
; hexadecimal.
;
start 0x0100
define c.ent 0x1263
define c.ret 0x126a
define c.ents 0x1116
define c.rets 0x112c

; concat's own code is the first three functions; the rest is library
main code 0x010e	; each argument opened and copied
copy code 0x01bc	; getl, then putl, until putl comes back 0
cantopen code 0x01fe	; name, ": can't open", exit
fopen code 0x0227
finit code 0x02b2
getl code 0x042a	; a line, or 512 characters, a block of the stream
putl code 0x058c	; a block to a stream; flushes on a newline, and returns
			; the flush's value then and the count otherwise
fputl code 0x05b0
fclose code 0x07a1
exit code 0x0dee
remark code 0x0ea9	; strings to a descriptor
