; names for /usr/bin/overstrike off the Micronix 1.6 distribution floppy,
; for src/tools/disas:
;
;	disas -f overstrike.ctl overstrike.dist
;
; There is no source for overstrike.  See README beside this file.  It has no
; header: the whole file is text at 0100, in Whitesmith's runtime, c.ent and
; c.ret the plain pair and c.ents and c.rets the pair that saves the register
; variable cells.  Every address here is written 0x...., which disas reads as
; hexadecimal.
;
start 0x0100
define c.ent 0x0ce9
define c.ret 0x0cf0
define c.ents 0x0b9c
define c.rets 0x0bb2

define ocol 0x0d1b	; the column the output has reached
define icol 0x0d1d	; the column the input has
define stdout 0x0d6b

; overstrike's own code is main alone; the rest is library
main code 0x010e	; one case for a backspace, and the catching up
getchar code 0x01ea
getc code 0x01f8
putchar code 0x0264
putc code 0x0292
exit code 0x08f3
