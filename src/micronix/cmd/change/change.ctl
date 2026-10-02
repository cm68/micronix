; names for /usr/bin/change off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f change.ctl change.dist
;
; There is no source for change.  See README beside this file.  It has the
; 99 94 header, text at 0100 for 244f bytes and data at 3000, and the older
; runtime login, td and include have: c.ent2 and c.ret2 are the register
; saving pair and the cells are r2, r3 and r4.  Every address here is
; written 0x...., which disas reads as hexadecimal.
;
start 0x0100
define c.ent 0x24f4
define c.ret 0x24fb
define c.ent2 0x24ff
define c.ret2 0x2514
define r4 0x6069
define r3 0xfffa
define r2 0x22e1

; the data
define line 0x3000	; 512
define sub 0x3200	; 512
define setbuf 0x3400	; 512
define out 0x3600	; 512, and after it
define head 0x3800	; the head of the pattern, and then
define tags 0x3802	; start and end of each tag, four bytes apiece

; change's own code is the first twenty functions, to 0b07 and main at 084f;
; the library follows from 13ef.  The strings are the messages, from 080d to
; 0838 and from 0abc to 0af0, and a "Missing trailing delimiter." at 0f68.
catsub code 0x0111	; the replacement for a match, onto the end of out
inset code 0x02be	; a character in a set
addnode code 0x030b	; a node on the pattern, or a closure on the last
mkset code 0x04c5	; the set a [ begins
addset code 0x06db	; a character into a set, once
strsave code 0x0733
clrset code 0x0784
initlist code 0x07b5	; calloc(8, 1) for the head
ident code 0x07cd	; the character, as it is: no escapes are translated
fatal code 0x07eb	; a message to the standard output, a newline, exit
main code 0x084f
append code 0x0a62	; a character onto the end of out
putline code 0x0aa5	; out to the standard output
makpat code 0x0b08	; the pattern into nodes
omatch code 0x0e25	; one node against one character
makesub code 0x0f84	; the replacement into its own form
amatch code 0x10fc	; the pattern from a node, at a character
fgets code 0x13ef
fputs code 0x1563
free code 0x1c16
calloc code 0x1a40
exit code 0x1d6f
strlen code 0x1ebd
strcpy code 0x1e58
