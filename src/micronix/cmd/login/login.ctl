; names for /bin/login off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f login.ctl login.dist
;
; There is no source for login.  This file names what the reconstruction in
; login.c had to know; see README beside this file.
;
; It is Whitesmith's C, the same runtime /etc/init uses - init.ctl names it
; there as c.ent/c.ret/c.ent2/c.ret2 and r1/r2/r3, and the two binaries
; agree.  Arguments are pushed right to left, so the last PUSH before a
; CALL is the first argument; the callee reads arg0 at (DE+4), arg1 at
; (DE+6), and returns its result in BC.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x.... .  An address
; written bare is parsed as decimal and the name lands somewhere else.
;
start 0x0100

; the runtime.  c.ent/c.ret are the plain prologue and epilogue of a leaf
; function; c.ent2/c.ret2 are the ones that save the three register
; variables.  A function whose first instruction is CALL c.ent2 is one
; that uses a register variable.
define c.ent 0x53c1
define c.ret 0x53c8
define c.ent2 0x53cc
define c.ret2 0x53e2

; Whitesmith's has exactly three register variables, in three fixed cells
; that the c.ent2 prologue saves and the c.ret2 epilogue restores.
define r1 0x655f
define r2 0x6561
define r3 0x6563

; login's own functions.  Each ends where the next begins.  These are the
; ones inside the program's own object; the C library is linked after it
; and starts at 0x29c2 (ctime's tables are the first thing in it).
code 0x015e
code 0x01da
code 0x0221
code 0x0322
code 0x0347
code 0x0460
code 0x04db
code 0x0532
code 0x0570
code 0x060c
code 0x075b
code 0x07aa
code 0x0803
code 0x085f
code 0x08f4
code 0x097c
code 0x09ab
code 0x09e5
code 0x0a39
code 0x0b2c
code 0x0b77
code 0x0bdb
code 0x0c37
code 0x0f0b
code 0x1035
code 0x1103
code 0x115d
code 0x119b
code 0x1201
code 0x12d3
code 0x190d
code 0x1a29
code 0x1a9b
code 0x1b64
code 0x1bd6
code 0x1e7b
code 0x1ff4
code 0x2062
code 0x2118
code 0x2220
code 0x2258
code 0x2411
code 0x256e
code 0x2681
code 0x2693
code 0x2712
code 0x2739
code 0x275d
code 0x2869

; login's string literals, at the head of its text
code 0x0111
code 0x011b
code 0x012c
code 0x013d
code 0x014b
code 0x0152
code 0x0212
code 0x021c
code 0x05e2
code 0x08d4
code 0x08e9
code 0x0a31
code 0x0c28
code 0x1151
