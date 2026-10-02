; names for /usr/bin/common off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f common.ctl common.dist
;
; There is no source for common.  See README beside this file.  It has no
; header: the whole file is text at 0100, in Whitesmith's runtime, c.ent and
; c.ret the plain pair and c.ents and c.rets the pair that saves the register
; variable cells.  Every address here is written 0x...., which disas reads as
; hexadecimal.
;
start 0x0100
define c.ent 0x17d1
define c.ret 0x17d8
define c.ents 0x1640
define c.rets 0x1656

define cmp 0x1c0d	; the last comparison, which sits just after the line buffers
define width 0x1c0f	; 80
define colw 0x1c11	; width over the number of columns wanted
define col1 0x1803
define col2 0x1805
define col3 0x1807
define file1 0x1809
define file2 0x180b
define line1 0x180d
define line2 0x1a0d

; common's own code is the functions to 05f8; the strings are the usage line at
; 0578 and ": can't open" at 05c2
main code 0x010e	; options, then the merge
merge code 0x012c	; one pass of the merge
compare code 0x019f	; <0, 0, >0, and the empty line is the end of its file
output code 0x0236	; what to write for the lines read
options code 0x02d9	; the arguments, and then the files opened
atoi code 0x04c2	; the number after -w, through btoi, handed 512
put code 0x04f2		; a line without its leading blanks
blanks code 0x0540
usage code 0x05aa
cantopen code 0x05d0
fopen code 0x05f9
finit code 0x0684
getl code 0x07fc
putc code 0x093e
exit code 0x0fcd
remark code 0x1088
btoi code 0x10ca
streq code 0x141d
prefix code 0x1398
idiv code 0x16a5
