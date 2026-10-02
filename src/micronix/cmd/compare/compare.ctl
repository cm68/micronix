; names for /usr/bin/compare off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f compare.ctl compare.dist
;
; There is no source for compare.  See README beside this file.  It has no
; header: the whole file is text at 0100, in Whitesmith's runtime, c.ent and
; c.ret the plain pair and c.ents and c.rets the pair that saves the register
; variable cells.  Every address here is written 0x...., which disas reads as
; hexadecimal.
;
start 0x0100
define c.ent 0x1df2
define c.ret 0x1df9
define c.ents 0x1a56
define c.rets 0x1a6c

; compare's own code is the first eight functions; the rest is library.  Its
; strings sit in front of main: "eof on file 2." at 010e, "eof on file 1." at
; 011d, "read" twice, the usage line at 0136 and ": can't open." at 0309.
main code 0x0152	; the two files, line by line
cantopen code 0x0317	; name, ": can't open.", exit
eofmsg code 0x033d	; a string and a newline: the "eof on file" messages
differ code 0x0359	; the line number, and the two lines
write code 0x0395	; to the standard output, by the length
die code 0x03bc		; to the standard error, then exit
ltoa code 0x03e5
fopen code 0x0426
fgets code 0x085e
exit code 0x13eb
remark code 0x14db	; strings to a descriptor
streq code 0x15eb
ltob code 0x16bb
_write code 0x1a2a
ladd code 0x1bc2
