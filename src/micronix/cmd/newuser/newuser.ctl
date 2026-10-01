; names for /bin/newuser off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f newuser.ctl newuser.dist
;
; There is no source for newuser.  This file names what the reconstruction in
; newuser.c had to know; see README beside this file.
;
; The binary has no header at all.  It is the "old cpm format" the kernel's
; exec accepts when the first word is not the object magic: the whole file is
; the text, loaded at 0100 and entered there.  disas has to be told that, so
; there is no start line to derive and nothing to relocate.
;
; It is Whitesmith's C, the same runtime login and init use.  c.ent/c.ret
; are the plain prologue and epilogue; c.ents/c.rets are the pair that save
; the three register variables, which live in the cells r2, r3 and r4.  A
; function that opens with CALL c.ents is one that uses a register variable.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0 or
; ends in h, so every address here is written 0x.... .
;
start 0x0100

define c.ent 0x3fc4
define c.ret 0x3fcb
define c.ents 0x3b03
define c.rets 0x3b19
define r2 0x4570
define r3 0x4572
define r4 0x4574

; newuser's own functions, in address order.  The names are what each one
; does; none is the author's.  The two marked "unreferenced" are called by
; nothing in the binary.
main code 0x010e
getname code 0x016f
newuser code 0x01a5
goodname code 0x01c2
userexists code 0x021a
removeuser code 0x0264	; unreferenced
adduser code 0x03a5
addpw code 0x056c
readpw code 0x0681
strsave code 0x06b0
writepw code 0x0709
putpw code 0x075b
putgstr code 0x086f
putstr code 0x0886
itoa code 0x089d
say code 0x08c6
findname code 0x08ee
prompt code 0x0941
ignoresigs code 0x09cb
readgr code 0x0a04
findgr code 0x0b96
grpname code 0x0bfb	; unreferenced
mkpath code 0x0c61
exists code 0x0dd7
dirname code 0x0e04
uidused code 0x0f0c
newuid code 0x0f89
showgr code 0x0fd4
askgroup code 0x10bf
rootonly code 0x1115
addgr code 0x113d
newgid code 0x11f9
writegr code 0x12c3
putgr code 0x1318
lock code 0x13fa
unlock code 0x14c8
stale code 0x14d6
grpnum code 0x1594

; the passwd reader and the stdio layer, which are library: getpwent.o and
; stdio from libS.a and libwsc.a.  getpwent and its helpers are the same
; module login carries (login.ctl, 0f0b..119b there).
getpwent code 0x15ee
fld code 0x16f6
getpwuid code 0x177a
getpwnam code 0x17c4
setpwent code 0x181e
endpwent code 0x185c
fopen code 0x187d
freopen code 0x18ef
fcreat code 0x195d
getfile code 0x1999
openfile code 0x1a15
finit code 0x1ad1
fclose code 0x1b49
fputs code 0x201b
calloc code 0x2546
malloc code 0x2611
exit code 0x2b54
memset code 0x2c44
perror code 0x2ecf
atoi code 0x2f99
blkcpy code 0x3297
streq code 0x32e7
strlen code 0x3264
cmpstr code 0x3332
instr code 0x334d
cpystr code 0x3527
itob code 0x358c
link code 0x3820
chown code 0x379f
close code 0x37c2
getuid code 0x37fb
access code 0x3768
mknod code 0x3843
open code 0x3871
read code 0x3895
seek code 0x38c1
signal code 0x38ec
sleep code 0x3a6f
stat code 0x3a7d
time code 0x3aa0
unlink code 0x3abf
write code 0x3ad7

; the strings, with the one byte in front of each that the disassembly read
; as an opcode left out
bytes 0x122 62
bytes 0x160 15
bytes 0x241 5
bytes 0x246 20
bytes 0x25a 10
bytes 0x314 17
bytes 0x326 27
bytes 0x341 61
bytes 0x37e 26
bytes 0x399 8
bytes 0x3a1 4
bytes 0x6eb 12
bytes 0x6f7 6
bytes 0x6fd 12
bytes 0x9c0 11
bytes 0x9ff 5
bytes 0xf65 36
bytes 0x101f 58
bytes 0x1059 34
bytes 0x107b 54
bytes 0x10b1 14
bytes 0x11e3 22
bytes 0x12a7 11
bytes 0x12b2 6
bytes 0x12b8 11
bytes 0x13ad 13
bytes 0x13ba 13
bytes 0x13c7 13
bytes 0x13d4 12
bytes 0x13e0 13
bytes 0x13ed 13
bytes 0x14bb 13
bytes 0x180d 5
bytes 0x1812 12
