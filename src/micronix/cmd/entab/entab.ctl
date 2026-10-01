; names for /usr/bin/entab off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f entab.ctl entab.dist
;
; There is no source for entab.  This file names what the reconstruction in
; entab.c had to know; see README beside this file.
;
; This one has the 99 94 header, a sixteen byte one, with the text at 0100 and
; the data at 2000, and the older of the two runtimes: the one login and
; init use, where c.ent2 and c.ret2 are the register saving pair and the
; cells are r2, r3 and r4.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x.... .
;
start 0x0100

define c.ent 0x0fd6
define c.ret 0x0fdd
define c.ent2 0x0fe1
define c.ret2 0x0ff6
define r2 0x2159
define r3 0x215b
define r4 0x215d

main code 0x010e	; the whole program: getc, then one case per character
catchup code 0x031b	; bring the output column up to the input's
_filbuf code 0x046d
_flsbuf code 0x0626
exit code 0x0bb8

; the data
define ocol 0x2000
define icol 0x2002
define c 0x2004
