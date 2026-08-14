; names for /bin/man off the Micronix 1.6 filesystem, for src/tools/disas
;
;	disas -f man.ctl man.dist
;
; There is no source for man.  The names below were read out of the code
; itself - see man.c beside this file, which is a reconstruction from this
; disassembly and uses the same names.
;
; It is Whitesmith's C, and it has the same shape as /etc/init: c.ent and
; c.ret at the top of the library, the three register variables in fixed
; cells, and the syscall stubs in the DATA segment rather than the text.
;
;	                man       init
;	c.switch      0x2256    0x3f62
;	c.ent         0x227b    0x3f87
;	c.ret         0x2282    0x3f8e
;	c.ent2        0x2286    0x3f92
;	c.ret2        0x229c    0x3fa8
;
; nm -v says: text 8662 bytes at 0x0100, data 525 at 0x3000, no bss.  With
; no bss the zeroed globals live in the data segment, which is why most of
; it reads as zeros in nm -b.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x....

start 0x0100

; ---------------------------------------------------------------------
; Whitesmith's runtime.  c.ent2 saves r1/r2/r3 into the frame - it pushes
; the cells at 0x3207, 0x3209 and 0x320b - and c.ret2 puts them back.
; c.switch walks a table of (target, value) pairs terminated by a zero
; target, and then takes the default.

define c.switch 0x2256
define c.ent 0x227b
define c.ret 0x2282
define c.ent2 0x2286
define c.ret2 0x229c

define r1 0x3207
define r2 0x3209
define r3 0x320b

; ---------------------------------------------------------------------
; The syscall stubs are in the data segment, reached by the indirect trap
; form - "cf 00" followed by the address of a stub, which is itself
; "cf <n>" and the argument words.  Named by the call number each makes.

define _sbrk 0x31c3
define _creat 0x31c7
define _exec 0x31cd
define _gtty 0x31d3
define _open 0x31d7
define _read 0x31dd
define _seek 0x31e3
define _stat 0x31e9
define _write 0x31ef

; ---------------------------------------------------------------------
; man's own text, 0x0116..0x0885.  Everything above that is library.

define main 0x0116
define doargs 0x0185		; the walk over argv
define show 0x0298		; find one page and hand it to form
define run 0x04f1		; fork, exec form, wait
define usage 0x05dd
define exists 0x060b
define issection 0x0636
define nodoc 0x0699		; "No documentation for ..."
define find 0x06c9		; search one directory for the page
define contains 0x07f4		; is the first string inside the second

; man's string literals.  Whitesmith's puts them in the text segment,
; which is how "0" at 0x0265 comes to be writable - see sectp below.
define s.help 0x0111
define s.dash 0x0183
define s.zero 0x0265
define s.slash1 0x0267
define s.manman1 0x0269
define s.dasht 0x0276
define s.form 0x0279
define s.slash2 0x027e
define s.manman2 0x0280
define s.usrhelp 0x028d
define s.usrbin 0x04e2
define s.bin 0x04eb
define s.usage1 0x05bd		; " [section] program ..."
define s.usage2 0x05d5		; "usage: "
define s.newline 0x0681
define s.nodoc 0x0683		; "No documentation for "
define s.dotdot 0x06bf
define s.dot 0x06c2
define s.read 0x06c4		; the fopen mode

; ---------------------------------------------------------------------
; library, named from what man calls it for and from what it reaches

define fopen 0x0886
define fclose 0x0b4f
define fread 0x0bee
define exit 0x155e
define equal 0x1647
define concat 0x1692
define putstr 0x16f7
define perror 0x19a0
define execv 0x1b54

; the syscall wrappers, each one a call to the matching stub above
define sbrk 0x1d82
define close 0x1d96
define creat 0x1da5
define exec 0x1dc9
define _exit 0x1dea
define fork 0x1df0
define gtty 0x1e03
define open 0x1e23
define read 0x1e47
define seek 0x1e73
define stat 0x1e9e
define wait 0x1ec1
define write 0x1ede

; ---------------------------------------------------------------------
; man's data segment.  There is no bss, so these are all here.

define istty 0x3000		; gtty(1) succeeded - pass -t to form
define helpmode 0x3002		; argv[0] contains "help"
define section 0x3004		; the section argument, if one was given
define argvec 0x3006		; the argv handed to form
define av 0x3016		; walks argvec while it is built
define scan 0x3018		; scratch pointer, cuts the suffix off a name
define found 0x301a		; what find() returned
define sectp 0x301c		; -> "0" at 0x0265, incremented through "9"
define dirent 0x301e		; a 16 byte directory entry, read raw
define dirname 0x3020		; its 14 name bytes
define direntz 0x302e		; the byte that terminates them
define errlist 0x3177		; perror's messages, 34 of them and a zero
define brk 0x31c1		; the initial break, 0x320d = end of data
