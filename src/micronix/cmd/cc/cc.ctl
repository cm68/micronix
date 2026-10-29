; names for /bin/cc off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f cc.ctl cc.dist
;
; There is no source for the distribution's cc.  See README.
; NOTE: addresses are written 0x.... so disas reads them in hex.
;
start 0x0100

; the string constants that sit in the text segment
bytes 0x0119 0x31		; /include/, a.out, /lib/ulib.a, /lib/mlib.a, pp, pp

; string constants embedded in the text, found by walking the byte
; runs that end in a NUL; the code on either side resumes at the next
; code label.
bytes 0x0198 0x54		; /lib/uhdr.o ... -i -cpm -ws -std ... a
bytes 0x0598 0x0f		; .s .8 .c .p .c
bytes 0x08d8 0x07		; -o ptc
bytes 0x0975 0x18		; -o -x -i unix -d cpp pp
bytes 0x0ab8 0x3d		; -c sh, | /bin/p1 ... /bin/pp
bytes 0x0bbf 0x42		; -c sh, | /bin/p1 ... /bin/an ... /bin/pp
bytes 0x0ccb 0x19		; -o -u -m -b0 -cem cp1 p1
bytes 0x0e5e 0x07		; cp2 p2
bytes 0x0fa7 0x06		; -o p3
bytes 0x106a 0x09		; -o as an
bytes 0x1159 0x11b		; /lib/libm.a ... link ld
bytes 0x17ed 0x10		; /bin/ cc cc \n space
bytes 0x2327 0x06		; /tmp/
bytes 0x2767 0x22c		; errno strings, perror

; the Whitesmiths C runtime prologue/epilogue, as in td.ctl
define c.ent 0x32e2
define c.ret 0x32e9

; the entry point's jump target is the crt startup; the function it
; calls with the arguments in DE is main.
main code 0x014a

; the second runtime pair, for functions with register variables
define c.ent2 0x32ed
define c.ret2 0x3303
define r2 0x4337
define r3 0x4339
define r4 0x433b

; the program's own functions
main code 0x014a
setup code 0x01ec
opts code 0x05a7
process code 0x08df
