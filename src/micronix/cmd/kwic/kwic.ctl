; names for /usr/bin/kwic off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f kwic.ctl kwic.dist
;
; There is no source for kwic.  This file names what the reconstruction in
; kwic.c had to know; see README beside this file.
;
; The binary has no header: it is the "old cpm format" the kernel's exec
; accepts, the whole file loaded as text at 0100 and entered there.  It is
; Whitesmith's C, the same runtime newuser, detab and the rest use: c.ent
; and c.ret are the plain prologue and epilogue, c.ents and c.rets the pair
; that saves the three register variable cells.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x.... .
;
start 0x0100
define c.ent 0x1075
define c.ret 0x107c
define c.ents 0x0f28
define c.rets 0x0f3e

; kwic's own code is the first three functions; everything after is library
main code 0x010e	; getl, then kwic, until the input is empty
kwic code 0x0168	; tabs to blanks, then a rotation for each word
rotate code 0x025f	; the word and what follows, '$', what came before
finit code 0x02ea	; attach a stream to a file descriptor
getl code 0x0442	; up to 512 characters, a newline included
putc code 0x05a4	; to the standard output
fputc code 0x05d2	; a negative character flushes and is not written
exit code 0x0c33
