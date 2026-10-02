; names for /usr/bin/include off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f include.ctl include.dist
;
; There is no source for include.  This file names what the reconstruction
; in include.c had to know; see README beside this file.
;
; It has the 99 94 header - text at 0100 for 18dd bytes, data at 2000 for
; 057f - and the older of the two runtimes, the one login, init, entab and
; td have: c.ent2 and c.ret2 are the register saving pair and the cells are
; r2, r3 and r4.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x.... .
;
start 0x0100
define c.ent 0x1982
define c.ret 0x1989
define c.ent2 0x198d
define c.ret2 0x19a2
define r2 0x2579
define r3 0x257b
define r4 0x257d

; the data
define stdin 0x2438
define stdout 0x2448
define linebuf 0x2000
define wordbuf 0x2200

; include's own code is the first five functions; the library follows
main code 0x0111		; include(0), and then exit
include code 0x012f		; one file, or the standard input for none
getword code 0x0225		; the first word of a string
cantread code 0x02b4		; name, ": can't read", exit
putline code 0x02dd		; fputs to the standard output
fopen code 0x02f4
getfile code 0x0410
openfile code 0x0482
finit code 0x054b
fclose code 0x05bd
fgets code 0x0712
fputs code 0x0886
_filbuf code 0x093a
exit code 0x10a4
streq code 0x118d
remark code 0x11d8		; strings to a descriptor
