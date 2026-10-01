; names for /bin/print off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f print.ctl print.dist
;
; There is no source for print.  This file names what the reconstruction in
; print.c had to know; see README beside this file.
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

define c.ent 0x474e
define c.ret 0x4755
define c.ents 0x417a
define c.rets 0x4190

; print's own functions, in address order.  The names are what each does;
; none is the author's.  Own code runs from 010f to 0ca4; the formatted
; output engine is at 0dbf and the library follows it.
main code 0x010f
getargs code 0x0207	; the options, and the page's arithmetic
usageexit code 0x03cc	; perror, then exit
printfile code 0x03e6	; every line of one file, then the last page
getline code 0x0410	; read a line and fold it into pieces
addcell code 0x05d2	; keep a piece; a full page is written
flush code 0x0625	; write the page gathered
blanks code 0x082a	; n newlines
header code 0x0863	; "%s Page %d %s"
puts code 0x0888
numeric code 0x08a8	; a string of digits
strsave code 0x08f3
nomem code 0x094e	; perror("pr"), exit
usage code 0x098f
putcell code 0x09a5	; a piece into a row, tabs expanded
atoi code 0x0adc
newpage code 0x0b0c	; blanks, page number, header, blanks
trailer code 0x0b36
getword code 0x0b50	; blanks and controls, then the printable run
swidth code 0x0be8	; display width
skipblank code 0x0c73
printf code 0x0ca5

; library
doprnt code 0x0dbf
getl code 0x1b09	; fgets by the length, newline included, no terminator
exit code 0x2672
perror code 0x30d5
btoi code 0x31dc	; the converter atoi hands 512 to
cpystr code 0x37ba
imul code 0x4324
idiv code 0x41df

; the strings
bytes 0x1f4 20
bytes 0x855 14
bytes 0x94b 3
bytes 0x964 42
