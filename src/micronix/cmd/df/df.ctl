; names for /bin/df off the Micronix 1.6 filesystem, for src/tools/disas
;
;	disas -f df.ctl df.dist
;
; There is no source for df.  The names below were read out of the code -
; see df.c beside this file, which is a reconstruction from this
; disassembly and uses the same names.
;
; It is Whitesmith's C: c.ent and c.ret at the top of the library, the
; three register variables in fixed cells, and the syscall stubs in the
; data segment rather than the text.  The strings live in the text
; segment, between the functions that use them, which is how the
; functions were found.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a
; 0 or ends in h, so every address here is written 0x....

start 0x0100

; ---------------------------------------------------------------------
; df's own text runs 0100..0935; everything above that is library.

define main 0x011a
define parse 0x01ed
define usage 0x0283
define process 0x02bc
define countfree 0x0395
define putnum 0x04dd
define putstr 0x0557
define isfs 0x0581
define statok 0x0706
define sequal 0x0731
define concat 0x077c
define strchr 0x07e1
define itoa 0x0863

; the strings, named by what they say
define devprefix 0x0111		; "/dev/"
define vflagarg 0x0117		; "-v"
define vflagarg2 0x01ea		; "-v" (a second copy, in parse)
define usagemsg 0x0269		; "usage: df filesystem ...\n"
define newline 0x02a3
define space 0x02a5
define notfs 0x02a7		; ": Not a file system\n"
define newline2 0x0365
define overflow 0x0367		; "Block count overflow\n"
define badfree 0x037d		; "Bad block in free list\n"
define space2 0x04db
define dotdot 0x057c		; ".."
define dot 0x057f			; "."

; ---------------------------------------------------------------------
; df's data segment

define vflag 0x1000
define sblock 0x1002		; the superblock: isize 0x1002, fsize
				; 0x1004, nfree 0x1006, free 0x1008
define name 0x11a1		; the current argument, main's register
define buf 0x11a5		; the "/dev/" concat buffer

; ---------------------------------------------------------------------
; Whitesmith's runtime and the library, named from the calls that reach
; them.  c.ent2 is the no-register-save entry, c.ent saves r1/r2/r3 and
; c.ret puts them back.

define c.ent 0x0e89
define c.ret 0x0e9f
define c.ent2 0x0e7e
define c.ret2 0x0e85

define exit 0x082d
define perror 0x0b64
define fprintf 0x0c2b
define strlen 0x0c6d
define close 0x0ca0
define open 0x0cb5
define read 0x0cd9
define seek 0x0d05
define stat 0x0d30
define write 0x0d53
