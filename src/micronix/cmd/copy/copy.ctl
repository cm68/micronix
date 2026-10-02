; names for /usr/bin/copy off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f copy.ctl copy.dist
;
; There is no source for copy.  See README beside this file.  It has no
; header: the whole file is text at 0100, in Whitesmith's runtime, c.ent and
; c.ret the plain pair and c.ents and c.rets the pair that saves the register
; variable cells.  Every address here is written 0x...., which disas reads as
; hexadecimal.
;
start 0x0100
define c.ent 0x0dde
define c.ret 0x0de5
define c.ents 0x0c91
define c.rets 0x0ca7

; copy's own code is main alone; the rest is library
main code 0x010e	; getchar, putchar, until the end of the file
finit code 0x0159	; attach a stream to a file descriptor
getchar code 0x02bf
getc code 0x02cd
putchar code 0x0339
exit code 0x09c8
